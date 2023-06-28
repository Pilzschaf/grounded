#include <grounded/grounded.h>

//TODO: Remove include
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

GROUNDED_FUNCTION_INLINE void groundedYield() {
    YieldProcessor();
    //Yield();
}