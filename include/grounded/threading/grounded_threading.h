#ifndef GROUNDED_THREADING_H
#define GROUNDED_THREADING_H

#include "../grounded.h"
#include "../memory/grounded_arena.h"
#include "../logger/grounded_logger.h"

#include <emmintrin.h> // For _mm_mfence

//TODO: Custom data in thread context. Maybe by using a user* data pointer
// or by using somehting similar to a BOOTSTRAP_PUH_STRUCT. As the user data could be directly
// stored in the scratch arena.

#define GROUNDED_HANDLE_ERROR_FUNCTION(name) void name(GroundedError error)
typedef GROUNDED_HANDLE_ERROR_FUNCTION(GroundedHandleErrorFunction);

typedef struct GroundedThreadContext {
    MemoryArena scratchArenas[2];
    GroundedLogFunction* logFunction;

    MemoryArena errorArena;
    ArenaMarker errorMarker;

    GroundedError lastError;
    GroundedHandleErrorFunction* unhandledErrorHandler;
} GroundedThreadContext;

GROUNDED_FUNCTION void threadContextInit(MemoryArena scratchArena0, MemoryArena scratchArena1, GroundedLogFunction* logFunction);

// Returns original log function
GROUNDED_FUNCTION GroundedLogFunction* threadContextSetLogFunction(GroundedLogFunction* logFunction);
GROUNDED_FUNCTION GroundedLogFunction* threadContextGetLogFunction();

GROUNDED_FUNCTION void threadContextClear();

GROUNDED_FUNCTION GroundedError* groundedPopError(MemoryArena* arena);
GROUNDED_FUNCTION void groundedFlushErrors();
GROUNDED_FUNCTION void groundedSetUnhandledErrorFunction(GroundedHandleErrorFunction* func);
GroundedHandleErrorFunction groundedDefaultUnhandledErrorHandler;

typedef void GroundedThread;

#define GROUNDED_THREAD_PROC(name) void name(void* userData)
typedef GROUNDED_THREAD_PROC(GroundedThreadProc);


//TODO: Maybe the arena to create the thread can also be used for its temp storage

// This thread can not and must not be managed by the creating thread. It frees resources automatically after it has finished executing
GROUNDED_FUNCTION void groundedDispatchThread(MemoryArena* arena, GroundedThreadProc* proc, void* userData, const char* threadName);

// This allocation must not be freed before the created thread has stopped
GROUNDED_FUNCTION GroundedThread* groundedStartThread(MemoryArena* arena, GroundedThreadProc* proc, void* userData, const char* threadName);
// Only safe to call once the thread has finished executing
GROUNDED_FUNCTION void groundedDestroyThread(GroundedThread* thread);

GROUNDED_FUNCTION bool groundedThreadWaitForFinish(GroundedThread* thread, u32 timeout); // Timeout in ms
GROUNDED_FUNCTION bool groundedThreadIsRunning(GroundedThread* thread);
GROUNDED_FUNCTION void groundedThreadRequestStop(GroundedThread* thread);
GROUNDED_FUNCTION bool groundedThreadShouldStop(GroundedThread* thread);
//TODO: Maybe implement a function which allows to get the current thread from somewhere in threadlocal storage?


//////////
// Mutexes
struct GroundedMutex;
typedef struct GroundedMutex GroundedMutex;

GROUNDED_FUNCTION_INLINE GroundedMutex groundedCreateMutex();
GROUNDED_FUNCTION_INLINE void groundedDestroyMutex(GroundedMutex* mutex);
GROUNDED_FUNCTION_INLINE void groundedLockMutex(GroundedMutex* mutex);
GROUNDED_FUNCTION_INLINE void groundedUnlockMutex(GroundedMutex* mutex);


/////////////
// Semaphores
struct GroundedSemaphore;
typedef struct GroundedSemaphore GroundedSemaphore;

// Max count might not be effective on every platform
GROUNDED_FUNCTION_INLINE GroundedSemaphore groundedCreateSemaphore(u32 initValue, u32 maxCount);
GROUNDED_FUNCTION_INLINE void groundedDestroySemaphore(GroundedSemaphore* semaphore);
// Also called Post semaphore
GROUNDED_FUNCTION_INLINE void groundedIncrementSemaphore(GroundedSemaphore* semaphore);
// Also called Wait semaphore
GROUNDED_FUNCTION_INLINE void groundedDecrementSemaphore(GroundedSemaphore* semaphore);


/////////////////////
// Condition Variable
struct GroundedConditionVariable;
typedef struct GroundedConditionVariable GroundedConditionVariable;

GROUNDED_FUNCTION_INLINE GroundedConditionVariable groundedCreateConditionVariable();
GROUNDED_FUNCTION_INLINE void groundedDestroyConditionVariable(GroundedConditionVariable* conditionVariable);
GROUNDED_FUNCTION_INLINE void groundedConditionVariableSignal(GroundedConditionVariable* conditionVariable);
// Waits until condition variable is signaled and releases the mutex while waiting, reacquiring it upon wakeup
GROUNDED_FUNCTION_INLINE void groundedConditionVariableWait(GroundedConditionVariable* conditionVariable, GroundedMutex* mutex);


/////////
// Fences
// All fences prevent compiler and cpu instruction reordering across the fence
// volatile disables register caching and rereads every time - When memory barriers are used volatile has no additional effect. 
// volatile DOES NOT imply ANY memory ordering constraints and is of fairly limited use in multithreaded code

// Full fence ensures no reads and writes can pass this fence
// This type of fence is quite costly and should only be used whilte testing.
// Nearly always there are cheaper options available
GROUNDED_FUNCTION_INLINE void groundedFullFence() {
    _mm_mfence();
}

// Write Release - when writing a shared value. Makes sure all reads and writes before it happen before it.
GROUNDED_FUNCTION_INLINE void groundedWriteReleaseFence() {
    _mm_sfence();
}

// Read Acquire - when reading a shared value. Says that all reads and writes after it must be executed after it
GROUNDED_FUNCTION_INLINE void groundedReadAcquireFence() {
    _mm_lfence();
}

// Pause tries to use special CPU instructions for more efficient busy looping. Should be used in hot busy loops.
GROUNDED_FUNCTION_INLINE void groundedPause() {
    _mm_pause();
}

// Yield can yield execution to another thread
GROUNDED_FUNCTION_INLINE void groundedYield();

GROUNDED_FUNCTION_INLINE void groundedSleep(u32 millis);

#ifdef _WIN32
#include "grounded_win32_threading.inl"
#else
#include "grounded_linux_threading.inl"
#endif

#endif // GROUNDED_THREADING_H
