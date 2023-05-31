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

#endif // GROUNDED_THREADING_H