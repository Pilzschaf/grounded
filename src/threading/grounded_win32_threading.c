#include <grounded/threading/grounded_threading.h>

#include <windows.h>

/////////////////
// Thread context
static DWORD threadContextIndex = 0;

GROUNDED_FUNCTION void threadContextInit(MemoryArena arena0, MemoryArena arena1, GroundedLogFunction* logFunction) {
    if(!threadContextIndex) {
        threadContextIndex = TlsAlloc();
    }
    ASSERT(threadContextIndex);
    GroundedThreadContext* threadContext = ARENA_PUSH_STRUCT(&arena0, GroundedThreadContext);
    TlsSetValue(threadContextIndex, threadContext);
    threadContext->scratchArenas[0] = arena0;
    threadContext->scratchArenas[1] = arena1;
    threadContext->logFunction = logFunction;
}

GROUNDED_FUNCTION MemoryArena* threadContextGetScratch(MemoryArena* conflictArena) {
    MemoryArena* result = 0;

    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(&threadContext->scratchArenas[0] == conflictArena) {
            result = &threadContext->scratchArenas[1];
        } else {
            result = &threadContext->scratchArenas[0];
        }
    }
    return result;
}

GROUNDED_FUNCTION GroundedLogFunction* threadContextSetLogFunction(GroundedLogFunction* logFunction) {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;

    GroundedLogFunction* previous = threadContext->logFunction;
    threadContext->logFunction = logFunction;
    return previous;
}

GROUNDED_FUNCTION GroundedLogFunction* threadContextGetLogFunction() {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    
    return threadContext->logFunction;
}

GROUNDED_FUNCTION void threadContextClear() {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    *threadContext = (GroundedThreadContext){0};
}