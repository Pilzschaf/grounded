#include "grounded_threading.h"

// Very simple async system. Tasks are handled asynchronously in order.

struct AsyncTaskData;

typedef u64 GroundedAsyncTask;

#define GROUNDED_ASYNC_PROC(name) void name(struct AsyncTaskData* task)
typedef GROUNDED_ASYNC_PROC(GroundedAsyncProc);

struct AsyncTaskData {
    GroundedAsyncProc* proc;
    u8* userData;
    u64 userDataSize;
};

struct AsyncNode {
    struct AsyncNode* next;
    struct AsyncTaskData data;
    GroundedAsyncTask task;
};

typedef struct GroundedAsyncSystem {
    GroundedConditionVariable taskAcquireConditionVariable;
    GroundedConditionVariable taskWaitConditionVariable;
    GroundedMutex mutex;
    GroundedAsyncTask nextTaskIndex;
    struct AsyncNode tasksSentinel;
    struct AsyncNode freeTasksSentinel;

    GroundedAsyncTask runningTask;

    // Circular buffer to store tasks
    GroundedCircularBufferReadWriter circularBuffer;
    GroundedThread* thread;

    MemoryArena arena;
} GroundedAsyncSystem;

GROUNDED_THREAD_PROC(asyncSystemThreadProc) {
    MemoryArena* scratch = threadContextGetScratch(0);

    GroundedAsyncSystem* system = (GroundedAsyncSystem*)userData;
    groundedLockMutex(&system->mutex);
    while(true) {
        while(!system->tasksSentinel.next) {
            if(groundedThreadShouldStop(system->thread)) {
                groundedUnlockMutex(&system->mutex);
                return;
            }
            groundedConditionVariableWait(&system->taskAcquireConditionVariable, &system->mutex);
        }
        struct AsyncNode* node = system->tasksSentinel.next;
        system->tasksSentinel.next = node->next;
        node->next = 0;
        system->runningTask = node->task;
        groundedUnlockMutex(&system->mutex);

        node->data.proc(&node->data);

        groundedLockMutex(&system->mutex);
        readFromCircularBuffer(&system->circularBuffer, node->data.userDataSize);
        system->runningTask = 0;
        node->next = system->freeTasksSentinel.next;
        system->freeTasksSentinel.next = node;
        groundedConditionVariableSignal(&system->taskWaitConditionVariable);
    }
    groundedUnlockMutex(&system->mutex);
}

GROUNDED_FUNCTION_INLINE bool createAsyncSystem(GroundedAsyncSystem* system, u64 ringBufferSize, u64 entryCount) {
    *system = (GroundedAsyncSystem){0};
    if(ringBufferSize == 0) ringBufferSize = KB(4);
    system->circularBuffer.buffer = groundedCreateCircularBuffer(ringBufferSize);
    system->circularBuffer.readHead = system->circularBuffer.buffer.buffer;
    system->circularBuffer.writeHead = system->circularBuffer.buffer.buffer;
    system->arena = createGrowingArena(osGetMemorySubsystem(), KB(64));
    struct AsyncNode* nodes = ARENA_PUSH_ARRAY(&system->arena, entryCount, struct AsyncNode);
    for(u64 i = 0; i < entryCount; ++i) {
        nodes[i].next = system->freeTasksSentinel.next;
        system->freeTasksSentinel.next = &nodes[i];
    }
    system->nextTaskIndex = 1;
    system->mutex = groundedCreateMutex();
    system->taskAcquireConditionVariable = groundedCreateConditionVariable();
    system->taskWaitConditionVariable = groundedCreateConditionVariable();
    //groundedDispatchThread(&arena, &asyncSystemThreadProc, system, "AsyncSystemThread");
    system->thread = groundedStartThread(&system->arena, &asyncSystemThreadProc, system, "AsyncSystemThread");
    
    return true;
}

GROUNDED_FUNCTION_INLINE void destroyAsyncSystem(GroundedAsyncSystem* system) {
    groundedThreadRequestStop(system->thread);
    // "Tickle" the thread by signaling the task acquire variable
    groundedConditionVariableSignal(&system->taskAcquireConditionVariable);
    
    groundedThreadWaitForFinish(system->thread, 0);
    groundedDestroyThread(system->thread);
    groundedDestroyCircularBuffer(&system->circularBuffer.buffer);
    groundedDestroyMutex(&system->mutex);
    groundedDestroyConditionVariable(&system->taskAcquireConditionVariable);
    groundedDestroyConditionVariable(&system->taskWaitConditionVariable);
}

//TODO: It would be possible to store the AsyncNodes in the circular buffer. This would remove the need for the freeSentinel and the additional buffer allocated form the arena

// Return 0 means that the task could not be pushed onto queue
GROUNDED_FUNCTION_INLINE GroundedAsyncTask groundedPushAsyncTask(GroundedAsyncSystem* system, GroundedAsyncProc* proc, void* userData, u64 userDataSize) {
    GroundedAsyncTask result = 0;
    groundedLockMutex(&system->mutex);

    struct AsyncNode* node = system->freeTasksSentinel.next;
    if(node) {
        node->data.userData = writeToCircularBuffer(&system->circularBuffer, (u8*)userData, userDataSize);
        if(node->data.userData) {
            system->freeTasksSentinel.next = node->next;
            node->next = 0;

            result = system->nextTaskIndex++;
            node->task = result;
            node->data.proc = proc;
            node->data.userDataSize = userDataSize;

            // Insert at back of tasks
            struct AsyncNode* taskNode = &system->tasksSentinel;
            while(taskNode->next) {
                taskNode = taskNode->next;
            }
            taskNode->next = node;
            groundedConditionVariableSignal(&system->taskAcquireConditionVariable);
        }
    }

    groundedUnlockMutex(&system->mutex);
    return result;
}

GROUNDED_FUNCTION_INLINE bool groundedAsyncTaskIsRunning(GroundedAsyncSystem* system, GroundedAsyncTask task) {
    ASSERT(false);
    return false;
}

GROUNDED_FUNCTION_INLINE bool groundedAsyncTaskIsPending(GroundedAsyncSystem* system, GroundedAsyncTask task) {
    ASSERT(false);
    return false;
}

GROUNDED_FUNCTION_INLINE bool _groundedAsyncIsTaskFinished(GroundedAsyncSystem* system, GroundedAsyncTask task) {
    bool result = true;
    if(system->runningTask == task) result = false;
    if(result) {
        struct AsyncNode* taskNode = &system->tasksSentinel;
        while(taskNode->next) {
            if(taskNode->task == task) {
                result = false;
                break;
            }
            taskNode = taskNode->next;
        }
    }
    return result;
}

GROUNDED_FUNCTION_INLINE bool groundedAsyncTaskWait(GroundedAsyncSystem* system, GroundedAsyncTask task, u64 timeout) {
    //TRACY_ZONE_HELPER(groundedAsyncTaskWait);

    //TODO: timeout
    groundedLockMutex(&system->mutex);
    while(!_groundedAsyncIsTaskFinished(system, task)) {
        groundedConditionVariableWait(&system->taskWaitConditionVariable, &system->mutex);
    }
    groundedUnlockMutex(&system->mutex);
    return true;
}

GROUNDED_FUNCTION_INLINE void groundedAsyncTaskCancel(GroundedAsyncSystem* system, GroundedAsyncTask task) {
    ASSERT(false);
}

GROUNDED_FUNCTION_INLINE bool groundedAsyncWaitForAll(GroundedAsyncSystem* system, u64 timeout) {
    //TRACY_ZONE_HELPER(groundedAsyncWaitForAll);

    groundedLockMutex(&system->mutex);
    while(system->runningTask || system->tasksSentinel.next) {
        groundedConditionVariableWait(&system->taskWaitConditionVariable, &system->mutex);
    }
    groundedUnlockMutex(&system->mutex);
    
    return true;
}