#include <grounded/threading/grounded_threading.h>

#include <pthread.h>
#include <semaphore.h>

/////////////////
// Thread context
static __thread GroundedThreadContext threadContext;

GROUNDED_FUNCTION void threadContextInit(MemoryArena arena0, MemoryArena arena1, GroundedLogFunction* logFunction) {
    threadContext.scratchArenas[0] = arena0;
    threadContext.scratchArenas[1] = arena1;
    threadContext.logFunction = logFunction;
}

GROUNDED_FUNCTION MemoryArena* threadContextGetScratch(MemoryArena* conflictArena) {
    if(&threadContext.scratchArenas[0] == conflictArena) {
        return &threadContext.scratchArenas[1];
    }
    return &threadContext.scratchArenas[0];
}

GROUNDED_FUNCTION GroundedLogFunction* threadContextSetLogFunction(GroundedLogFunction* logFunction) {
    GroundedLogFunction* previous = threadContext.logFunction;
    threadContext.logFunction = logFunction;
    return previous;
}

GROUNDED_FUNCTION GroundedLogFunction* threadContextGetLogFunction() {
    return threadContext.logFunction;
}

GROUNDED_FUNCTION void threadContextClear() {
    threadContext = (GroundedThreadContext){0};
}