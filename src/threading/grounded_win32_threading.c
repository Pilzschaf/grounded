#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

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
    threadContext->errorArena = createFixedSizeArena(osGetMemorySubsystem(), KB(8));
    threadContext->errorMarker = arenaCreateMarker(&threadContext->errorArena);
    threadContext->unhandledErrorHandler = &groundedDefaultUnhandledErrorHandler;
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

////////
// Error
GROUNDED_FUNCTION void groundedPushError(String8 str, String8 filename, u64 line) {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(!str8IsEmpty(threadContext->lastError.text)) {
            // We already have an error so print it!
            groundedFlushErrors();
        }
        threadContext->lastError.filename = filename;
        threadContext->lastError.line = line;
        threadContext->lastError.text = str8Copy(&threadContext->errorArena, str);
    }
}

// Predeclare str8FromFormatVaList from grounded_string so we do not have to include stdarg.h in our headers
String8 str8FromFormatVaList(struct MemoryArena* arena, const char* format, va_list args);
GROUNDED_FUNCTION void groundedPushErrorf(String8 filename, u64 line, const char* fmt, ...) {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(!str8IsEmpty(threadContext->lastError.text)) {
            // We already have an error so print it!
            groundedFlushErrors();
        }
        va_list args;
        va_start(args, fmt);
        String8 str = str8FromFormatVaList(&threadContext->errorArena, fmt, args);
        groundedPushError(str, filename, line);
        va_end(args);
    }
}

GROUNDED_FUNCTION bool groundedHasError() {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        return !str8IsEmpty(threadContext->lastError.text);
    }
    return false;
}

GROUNDED_FUNCTION GroundedError* groundedPopError(MemoryArena* arena) {
    GroundedError* result = 0;
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(!str8IsEmpty(threadContext->lastError.text)) {
            result = ARENA_PUSH_STRUCT_NO_CLEAR(arena, GroundedError);
            *result = threadContext->lastError;
            result->text = str8CopyAndNullTerminate(arena, threadContext->lastError.text);
            threadContext->lastError.text = EMPTY_STRING8;
            arenaResetToMarker(threadContext->errorMarker);
        }
    }
    return result;
}

GROUNDED_FUNCTION void groundedFlushErrors() {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(threadContext->unhandledErrorHandler && groundedHasError()) {
            threadContext->unhandledErrorHandler(threadContext->lastError);
            threadContext->lastError.text = EMPTY_STRING8;
        }
        arenaResetToMarker(threadContext->errorMarker);
    }
}

GROUNDED_FUNCTION void groundedSetUnhandledErrorFunction(GroundedHandleErrorFunction* func) {
    void* tlsMemory = TlsGetValue(threadContextIndex);
    GroundedThreadContext* threadContext = (GroundedThreadContext*) tlsMemory;
    if(threadContext) {
        if(threadContext->unhandledErrorHandler) {
            groundedFlushErrors();
        }
        threadContext->unhandledErrorHandler = func;
    }
}

GROUNDED_HANDLE_ERROR_FUNCTION(groundedDefaultUnhandledErrorHandler) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    String8 message = str8FromFormat(scratch, "Unhandled error: %.*s", error.text.size, error.text.base);
    threadContextGetLogFunction()(str8GetCstr(scratch, message), GROUNDED_LOG_LEVEL_ERROR, error.filename, error.line);
    arenaEndTemp(temp);
}


struct Win32Thread {
    HANDLE thread;
    volatile bool stopRequested;
    GroundedThreadProc* proc;
    void* userData;
    MemoryArena* arena;
    ArenaMarker marker;
};

static DWORD win32ThreadProc(void* arg) {
	struct Win32Thread* thread = (struct Win32Thread*)arg;

    GroundedThreadContext* threadContext = ARENA_PUSH_STRUCT(thread->arena, GroundedThreadContext);
    TlsSetValue(threadContextIndex, threadContext);
    threadContext->scratchArenas[0] = *thread->arena;

    thread->proc(thread->userData);
	return 0;
}


GROUNDED_FUNCTION GroundedThread* groundedStartThread(MemoryArena* arena, GroundedThreadProc* proc, void* userData, const char* threadName) {
    ArenaMarker marker = arenaCreateMarker(arena);
    struct Win32Thread* result = ARENA_PUSH_STRUCT(arena, struct Win32Thread);
    result->marker = marker;

    result->proc = proc;
    result->userData = userData;
    result->arena = arena;

    result->thread = CreateThread(0, 0, win32ThreadProc, result, 0, 0);

    if(result && threadName) {
        //TODO: Name thread
        // SetThreadDescription
    }

    return result;
}

GROUNDED_FUNCTION void groundedDestroyThread(GroundedThread* opaqueThread) {
    struct Win32Thread* thread = (struct Win32Thread*) opaqueThread;
    ASSERT(!groundedThreadIsRunning(opaqueThread));

    CloseHandle(thread->thread);
    arenaResetToMarker(thread->marker);
}


GROUNDED_FUNCTION bool groundedThreadWaitForFinish(GroundedThread* opaqueThread, u32 timeout) {
    struct Win32Thread* thread = (struct Win32Thread*) opaqueThread;
    if(timeout == 0) timeout = INFINITE;
    DWORD result = WaitForSingleObject(thread->thread, timeout);
    if(result == WAIT_OBJECT_0) {
        return true;
    } 
    return false;
}

GROUNDED_FUNCTION bool groundedThreadIsRunning(GroundedThread* opaqueThread) {
    struct Win32Thread* thread = (struct Win32Thread*) opaqueThread;
    DWORD result = WaitForSingleObject(thread->thread, 0);
    if(result == WAIT_OBJECT_0) {
        return false;
    }
    return true;
}

GROUNDED_FUNCTION void groundedThreadRequestStop(GroundedThread* opaqueThread) {
    struct Win32Thread* thread = (struct Win32Thread*) opaqueThread;
    thread->stopRequested = true;
    groundedWriteReleaseFence();
}

GROUNDED_FUNCTION bool groundedThreadShouldStop(GroundedThread* opaqueThread) {
    struct Win32Thread* thread = (struct Win32Thread*) opaqueThread;
    groundedReadAcquireFence();
    return thread->stopRequested;
}
