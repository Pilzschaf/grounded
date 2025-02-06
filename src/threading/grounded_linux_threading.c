#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>

/////////////////
// Thread context
static __thread GroundedThreadContext threadContext;

GROUNDED_FUNCTION void threadContextInit(MemoryArena arena0, MemoryArena arena1, GroundedLogFunction* logFunction) {
    threadContext.scratchArenas[0] = arena0;
    threadContext.scratchArenas[1] = arena1;
    threadContext.logFunction = logFunction;
    threadContext.errorArena = createFixedSizeArena(osGetMemorySubsystem(), KB(8));
    threadContext.errorMarker = arenaCreateMarker(&threadContext.errorArena);
    threadContext.unhandledErrorHandler = &groundedDefaultUnhandledErrorHandler;
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

// Predeclare str8FromFormatVaList from grounded_string so we do not have to include stdarg.h in our headers
String8 str8FromFormatVaList(struct MemoryArena* arena, const char* format, va_list args);

//////////
// Logging
GROUNDED_FUNCTION void logFunctionf(GroundedLogLevel level, String8 filename, u64 line, const char* fmt, ...) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    va_list args;
    va_start(args, fmt);
    String8 str = str8FromFormatVaList(scratch, fmt, args);
    threadContextGetLogFunction()((const char*)str.base, level, filename, line);
    va_end(args);

    arenaEndTemp(temp);
}

////////
// Error
GROUNDED_FUNCTION void groundedPushError(String8 str, String8 filename, u64 line) {
    if(!str8IsEmpty(threadContext.lastError.text)) {
        // We already have an error so print it!
        groundedFlushErrors();
    }
    threadContext.lastError.filename = filename;
    threadContext.lastError.line = line;
    threadContext.lastError.text = str8Copy(&threadContext.errorArena, str);
}

GROUNDED_FUNCTION void groundedPushErrorf(String8 filename, u64 line, const char* fmt, ...) {
    if(!str8IsEmpty(threadContext.lastError.text)) {
        // We already have an error so print it!
        groundedFlushErrors();
    }
    va_list args;
    va_start(args, fmt);
    String8 str = str8FromFormatVaList(&threadContext.errorArena, fmt, args);
    groundedPushError(str, filename, line);
    va_end(args);
}

GROUNDED_FUNCTION bool groundedHasError() {
    return !str8IsEmpty(threadContext.lastError.text);
}

GROUNDED_FUNCTION GroundedError* groundedPopError() {
    GroundedError* result = 0;
    if(!str8IsEmpty(threadContext.lastError.text)) {
        result = &threadContext.lastError;
        threadContext.lastError.text = EMPTY_STRING8;
        arenaResetToMarker(threadContext.errorMarker);
    }
    return result;
}

GROUNDED_FUNCTION void groundedFlushErrors() {
    if(threadContext.unhandledErrorHandler && groundedHasError()) {
        threadContext.unhandledErrorHandler(threadContext.lastError);
        threadContext.lastError.text = EMPTY_STRING8;
    }
    arenaResetToMarker(threadContext.errorMarker);
}

GROUNDED_FUNCTION void groundedSetUnhandledErrorFunction(GroundedHandleErrorFunction* func) {
    if(threadContext.unhandledErrorHandler) {
        groundedFlushErrors();
    }
    threadContext.unhandledErrorHandler = func;
}

GROUNDED_HANDLE_ERROR_FUNCTION(groundedDefaultUnhandledErrorHandler) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    String8 message = str8FromFormat(scratch, "Unhandled error: %.*s", error.text.size, error.text.base);
    threadContextGetLogFunction()(str8GetCstr(scratch, message), GROUNDED_LOG_LEVEL_ERROR, str8GetCstr(scratch, error.filename), error.line);
    arenaEndTemp(temp);
}

struct LinuxThread {
    pthread_t thread;
    pthread_mutex_t terminateMutex;
    pthread_cond_t terminateCond; // Cond variable so we can easily block to wait for it
    volatile bool terminated;
    volatile bool stopRequested;
    GroundedThreadProc* proc;
    void* userData;
    MemoryArena* arena;
    ArenaMarker marker;
};

static void signalTermination(void* arg) {
    struct LinuxThread* thread = (struct LinuxThread*) arg;
    pthread_mutex_lock(&thread->terminateMutex);
    thread->terminated = true;
    pthread_cond_signal(&thread->terminateCond);
    pthread_mutex_unlock(&thread->terminateMutex);
}

static void* linuxThreadProc(void* arg) {
    struct LinuxThread* thread = (struct LinuxThread*) arg;

    pthread_cleanup_push(signalTermination, thread);

    // Clear thread context. Probably unnecessary
    threadContextClear();
    threadContext.errorArena = createFixedSizeArena(osGetMemorySubsystem(), KB(8));
    threadContext.errorMarker = arenaCreateMarker(&threadContext.errorArena);
    threadContext.scratchArenas[0] = *thread->arena;
    threadContext.unhandledErrorHandler = groundedDefaultUnhandledErrorHandler;

    thread->proc(thread->userData);

    // Flush possible errors
    groundedFlushErrors();
    arenaRelease(&threadContext.errorArena);

    // Remove cleanup handler and call it
    pthread_cleanup_pop(1);
    return 0;
}

GROUNDED_FUNCTION GroundedThread* groundedStartThread(MemoryArena* arena, GroundedThreadProc* proc, void* userData, const char* threadName) {
    ArenaMarker marker = arenaCreateMarker(arena);
    struct LinuxThread* result = ARENA_PUSH_STRUCT(arena, struct LinuxThread);
    result->marker = marker;

    pthread_attr_t threadAttributes;
    if(result) {
        int errors = pthread_attr_init(&threadAttributes);
        if(errors != 0) {
            result = 0;
        }
    }

    // This can be used to start threads in detached state. This makes them not joinable
    if(result) {
        int errors = pthread_attr_setdetachstate(&threadAttributes, PTHREAD_CREATE_DETACHED);
        if(errors != 0) {
            result = 0;
        }
    }

    if(result) {
        pthread_mutex_init(&result->terminateMutex, 0);
        pthread_cond_init(&result->terminateCond, 0);
        result->proc = proc;
        result->userData = userData;
        result->arena = arena;
    }

    //TODO: This can be used in the new thread to signal the terminated cond variable. This is useful because it also handles cancelation! But we probably dont want to support cancelation
    // But it does not handle return from the thread proc. So there should also be a set to the cond variable in all cases
    //pthread_cleanup_push();

    if(result) {
        if(pthread_create(&result->thread, &threadAttributes, linuxThreadProc, result) != 0) {
            result = 0;
        }
    }

    

    if(result && threadName) {
        //TODO: Seems like linux only supports up to 16 characters?
        pthread_setname_np(result->thread, threadName);
#ifdef TRACY_ENABLE
        __attribute__((visibility("default"))) void ___tracy_set_thread_name( const char* name );
        ___tracy_set_thread_name(threadName);
#endif
    }

    return result;
}

GROUNDED_FUNCTION void groundedDestroyThread(GroundedThread* opaqueThread) {
    struct LinuxThread* thread = (struct LinuxThread*) opaqueThread;
    ASSERT(!groundedThreadIsRunning(opaqueThread));
    pthread_mutex_destroy(&thread->terminateMutex);
    pthread_cond_destroy(&thread->terminateCond);
    arenaResetToMarker(thread->marker);
}

GROUNDED_FUNCTION bool groundedThreadWaitForFinish(GroundedThread* opaqueThread, u32 timeout) {
    struct LinuxThread* thread = (struct LinuxThread*) opaqueThread;
    //TODO: Timeout
    //struct timespec;
    //pthread_cond_timedwait(&thread->terminated, 0, )
    pthread_mutex_lock(&thread->terminateMutex);
    while(!thread->terminated) {
        pthread_cond_wait(&thread->terminateCond, &thread->terminateMutex);
    }
    pthread_mutex_unlock(&thread->terminateMutex);

    return true;
}

GROUNDED_FUNCTION bool groundedThreadIsRunning(GroundedThread* opaqueThread) {
    struct LinuxThread* thread = (struct LinuxThread*) opaqueThread;
    return !thread->terminated;
}

GROUNDED_FUNCTION void groundedThreadRequestStop(GroundedThread* opaqueThread) {
    struct LinuxThread* thread = (struct LinuxThread*) opaqueThread;
    thread->stopRequested = true;
    groundedWriteReleaseFence();
}

GROUNDED_FUNCTION bool groundedThreadShouldStop(GroundedThread* opaqueThread) {
    struct LinuxThread* thread = (struct LinuxThread*) opaqueThread;
    groundedReadAcquireFence();
    return thread->stopRequested;
}