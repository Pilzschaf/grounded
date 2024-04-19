#include "grounded_threading.h"

#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

GROUNDED_FUNCTION_INLINE void groundedYield() {
    sched_yield();
}

GROUNDED_FUNCTION_INLINE void groundedSleep(u32 millis) {
    usleep(millis * 1000);
}

struct GroundedMutex {
    pthread_mutex_t mutex;
};

GROUNDED_FUNCTION_INLINE GroundedMutex groundedCreateMutex() {
    GroundedMutex result = {0};
    pthread_mutex_init(&result.mutex, 0);

    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroyMutex(GroundedMutex* mutex) {
    pthread_mutex_destroy(&mutex->mutex);
}

GROUNDED_FUNCTION_INLINE void groundedLockMutex(GroundedMutex* mutex) {
    pthread_mutex_lock(&mutex->mutex);
}

GROUNDED_FUNCTION_INLINE void groundedUnlockMutex(GroundedMutex* mutex) {
    pthread_mutex_unlock(&mutex->mutex);
}


struct GroundedSemaphore {
    sem_t semaphore;
};

GROUNDED_FUNCTION_INLINE GroundedSemaphore groundedCreateSemaphore(u32 initValue, u32 maxCount) {
    GroundedSemaphore result = {0};

    sem_init(&result.semaphore, 0, initValue);
    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroySemaphore(GroundedSemaphore* semaphore) {
    sem_destroy(&semaphore->semaphore);
}

GROUNDED_FUNCTION_INLINE void groundedIncrementSemaphore(GroundedSemaphore* semaphore) {
    sem_post(&semaphore->semaphore);
}

GROUNDED_FUNCTION_INLINE void groundedDecrementSemaphore(GroundedSemaphore* semaphore) {
    sem_wait(&semaphore->semaphore);
}

struct GroundedConditionVariable {
    pthread_cond_t conditionVariable;
};

GROUNDED_FUNCTION_INLINE GroundedConditionVariable groundedCreateConditionVariable() {
    GroundedConditionVariable result = {0};

    pthread_cond_init(&result.conditionVariable, 0);
    return result;
}

GROUNDED_FUNCTION_INLINE void groundedDestroyConditionVariable(GroundedConditionVariable* conditionVariable) {
    pthread_cond_destroy(&conditionVariable->conditionVariable);
}

GROUNDED_FUNCTION_INLINE void groundedConditionVariableSignal(GroundedConditionVariable* conditionVariable) {
    pthread_cond_signal(&conditionVariable->conditionVariable);
}

GROUNDED_FUNCTION_INLINE void groundedConditionVariableWait(GroundedConditionVariable* conditionVariable, GroundedMutex* mutex) {
    pthread_cond_wait(&conditionVariable->conditionVariable, &mutex->mutex);
}

// Returns incremented value eg. a pre increment
GROUNDED_FUNCTION_INLINE u64 groundedInterlockedIncrement64(volatile u64* value) {
    return __sync_add_and_fetch(value, 1);
}

// Note that the compare exchange intrinsics can't detect ABA problems!
// Returns initial value of value
GROUNDED_FUNCTION_INLINE u64 groundedInterlockedCompareExchange(volatile u64* value, u64 originalValue, u64 newValue) {
    return __sync_val_compare_and_swap(value, originalValue, newValue);
}