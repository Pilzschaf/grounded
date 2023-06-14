#ifndef GROUNDED_THREADING_H
#define GROUNDED_THREADING_H

#include "../grounded.h"
#include "../memory/grounded_arena.h"
#include "../logger/grounded_logger.h"

//TODO: Custom data in thread context. Maybe by using a user* data pointer
// or by using somehting similar to a BOOTSTRAP_PUH_STRUCT. As the user data could be directly
// stored in the scratch arena.

typedef struct GroundedThreadContext {
    MemoryArena scratchArenas[2];
    GroundedLogFunction* logFunction;
} GroundedThreadContext;

GROUNDED_FUNCTION void threadContextInit(MemoryArena scratchArena0, MemoryArena scratchArena1, GroundedLogFunction* logFunction);
// If the current function already uses an arena for persisting allocations, it should be passed as conflictArena
// The returned arena should be used for temporary allocations that are reset in the same scope (temporary stack based allocations)
GROUNDED_FUNCTION MemoryArena* threadContextGetScratch(MemoryArena* conflictArena);

// Returns original log function
GROUNDED_FUNCTION GroundedLogFunction* threadContextSetLogFunction(GroundedLogFunction* logFunction);
GROUNDED_FUNCTION GroundedLogFunction* threadContextGetLogFunction();

GROUNDED_FUNCTION void threadContextClear();


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

#endif // GROUNDED_THREADING_H