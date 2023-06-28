#include <grounded/grounded.h>

#include <pthread.h>

GROUNDED_FUNCTION_INLINE void groundedYield() {
    sched_yield();
}
