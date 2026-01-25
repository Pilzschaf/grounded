#include <grounded/grounded.h>

//TODO: Remove include
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

struct GroundedMutex {
    CRITICAL_SECTION mutex;
};

GROUNDED_FUNCTION_INLINE GroundedMutex groundedCreateMutex() {
    GroundedMutex result = {0};
    InitializeCriticalSection(&result.mutex);
    // CreateMutexA(0, FALSE, 0);
    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroyMutex(GroundedMutex* mutex) {
    DeleteCriticalSection(&mutex->mutex);
    //CloseHandle(mutex->mutex)
}

GROUNDED_FUNCTION_INLINE void groundedLockMutex(GroundedMutex* mutex) {
    EnterCriticalSection(&mutex->mutex);
    //WaitForSingleObject(mutex->mutex, INFINITE);
}

GROUNDED_FUNCTION_INLINE void groundedUnlockMutex(GroundedMutex* mutex) {
    LeaveCriticalSection(&mutex->mutex);
    //ReleaseMutex(mutex->mutex)
}

struct GroundedSemaphore {
    HANDLE semaphore;
};

GROUNDED_FUNCTION_INLINE GroundedSemaphore groundedCreateSemaphore(u32 initValue, u32 maxCount) {
    GroundedSemaphore result = {0};
    result.semaphore = CreateSemaphoreA(0, initValue, maxCount, 0);
    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroySemaphore(GroundedSemaphore* semaphore) {
    CloseHandle(semaphore->semaphore);
}

GROUNDED_FUNCTION_INLINE void groundedIncrementSemaphore(GroundedSemaphore* semaphore) {
    ReleaseSemaphore(semaphore->semaphore, 1, 0);
}

GROUNDED_FUNCTION_INLINE void groundedDecrementSemaphore(GroundedSemaphore* semaphore) {
    WaitForSingleObject(semaphore->semaphore, INFINITE);
}

struct GroundedConditionVariable {
    CONDITION_VARIABLE conditionVariable;
};

GROUNDED_FUNCTION_INLINE GroundedConditionVariable groundedCreateConditionVariable() {
    GroundedConditionVariable result = {0};
    InitializeConditionVariable(&result.conditionVariable);
    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroyConditionVariable(GroundedConditionVariable* conditionVariable) {
    //TODO: Nothing to do?
}

GROUNDED_FUNCTION_INLINE void groundedConditionVariableSignal(GroundedConditionVariable* conditionVariable) {
    WakeConditionVariable(&conditionVariable->conditionVariable);
}

GROUNDED_FUNCTION_INLINE void groundedConditionVariableWait(GroundedConditionVariable* conditionVariable, GroundedMutex* mutex) {
    //TODO: Timed wait
    SleepConditionVariableCS(&conditionVariable->conditionVariable, &mutex->mutex, INFINITE);
}

GROUNDED_FUNCTION_INLINE void groundedYield() {
    YieldProcessor();
    //Yield();
}

GROUNDED_FUNCTION_INLINE void groundedSleep(u32 millis) {
    Sleep(millis);
}
