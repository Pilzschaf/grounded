#include <grounded/grounded.h>

#include <pthread.h>
#include <unistd.h>

GROUNDED_FUNCTION_INLINE void groundedYield() {
    sched_yield();
}

GROUNDED_FUNCTION_INLINE void groundedSleep(u32 millis) {
    usleep(millis * 1000);
}