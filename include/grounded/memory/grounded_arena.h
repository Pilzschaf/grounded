#ifndef GROUNDED_ARENA_H
#define GROUNDED_ARENA_H

#include "../grounded.h"
#include "../string/grounded_string.h"
#include "grounded_memory.h"

// Enables all simple arena debug mechanisms project wide
#ifdef GROUNDED_ARENA_DEBUG
#define GROUNDED_ARENA_OVERWRITE_WITH_SCRATCH
#define GROUNDED_ARENA_TEMP_MEMORY_CHECK
#endif

//////////////////
// Public User API

// The idea of the memory arena is to be a very fast stack based allocator 
// which supports a variety of different implementations. For maximum performance
// the hot path of ARENA_PUSH is completely inlined. Only growing and shrinking
// are not inlined and can be set to arbitrary functions by each arena implementation
// Users should not expect any specific pattern in the location of consecutive allocations in memory
// Debugging functionality can optionally be enabled by filling the debugFunctions pointer

typedef struct MemoryArena MemoryArena;

// Allocation macros
#define ARENA_PUSH_STRUCT(arena, type) (type*)_arenaPushSize(arena, sizeof(type), ALIGNMENT_OF(type), true, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_STRUCT_ALIGNED(arena, type, alignment) (type*)_arenaPushSize(arena, sizeof(type), alignment, true, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_ARRAY(arena, count, type) (type*)_arenaPushSize(arena, sizeof(type)*(count), ALIGNMENT_OF(type), true, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_ARRAY_ALIGNED(arena, count, type, alignment) (type*)_arenaPushSize(arena, sizeof(type)*(count), alignment, true, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_BOOTSTRAP(arena) (MemoryArena*)_arenaBootstrap(arena, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_BOOTSTRAP_PUSH_STRUCT(arena, type, member) (type*)_arenaBootstrapPushSize(arena, sizeof(type), OFFSET_OF_MEMBER(type, member), ALIGNMENT_OF(type), __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_STRUCT_NO_CLEAR(arena, type) (type*)_arenaPushSize(arena, sizeof(type), ALIGNMENT_OF(type), false, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_STRUCT_NO_CLEAR_ALIGNED(arena, type, alignment) (type*)_arenaPushSize(arena, sizeof(type), alignment, false, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_ARRAY_NO_CLEAR(arena, count, type) (type*)_arenaPushSize(arena, sizeof(type)*(count), ALIGNMENT_OF(type), false, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_ARRAY_NO_CLEAR_ALIGNED(arena, count, type, alignment) (type*)_arenaPushSize(arena, sizeof(type)*(count), alignment, false, __LINE__, STR8_LITERAL(__FILE__))
#define ARENA_PUSH_COPY(arena, type, original) (type*)_arenaPushCopy(arena, sizeof(type), ALIGNMENT_OF(type), original, __LINE__, STR8_LITERAL(__FILE__))

// Freeing functions
GROUNDED_FUNCTION_INLINE void arenaPopTo(MemoryArena* arena, u8* newHead);
GROUNDED_FUNCTION_INLINE void arenaRelease(MemoryArena* arena);

// Debugging functionality
GROUNDED_FUNCTION void enableDebugMemoryLoggingForArena(MemoryArena* arena);
GROUNDED_FUNCTION void enableDebugMemoryOverflowDetectForArena(MemoryArena* arena);
GROUNDED_FUNCTION void enableDebugMemoryUnderflowDetectForArena(MemoryArena* arena);

// Temporary Memroy and Arena Markers basically do the same thing.
// However both variants exist so the user can express his intent more precisely.
// Temporary memory should always be used inside a single function scope.
// It is only allowed to be used in a strict stack-based fasion.
// If a reset point should be stored across function scopes, an arena marker should be used.
// Temporary Memory
typedef struct {
    MemoryArena* arena;
    // base and pos to be able to differentiate between end of buffer and start of next buffer
    u8* base;
    u64 pos;
#ifdef GROUNDED_ARENA_TEMP_MEMORY_CHECK
    u32 tempStackIndex;
#endif
} ArenaTempMemory;
GROUNDED_FUNCTION_INLINE ArenaTempMemory arenaBeginTemp(MemoryArena* arena);
GROUNDED_FUNCTION_INLINE void arenaEndTemp(ArenaTempMemory temp);

// Arena markers
typedef struct {
    MemoryArena* arena;
    u8* base;
    u64 pos;
} ArenaMarker;
GROUNDED_FUNCTION_INLINE ArenaMarker arenaCreateMarker(MemoryArena* arena);
GROUNDED_FUNCTION_INLINE void arenaResetToMarker(ArenaMarker marker);




///////////////////
// Implementors API

// Size should point to the request size and is filled out with the actual returned size.
// If return is non-null size is guaranteed to be >= requested size.
// Memory returned may not be zeroed
// Alignment is always <= 4096
typedef void* ArenaGrowFunc(MemoryArena* arena, u64* size, u64 alignment);
// Is called once a reset point is out of current block which would also mean it is never called for a single contigous arena. 
// If this is called with a null pointer the arena should be completely reset and not be used afterwards
typedef void ArenaShrinkFunc(MemoryArena* arena, u8* newHead);

// Memory Debugging functionality. If those are present they are called for every single allocation
typedef void* ArenaDebugAllocate(MemoryArena* arena, u64 size, u64 alignment, bool clear, u64 line, String8 filename);
typedef void ArenaDebugDeallocate(MemoryArena* arena, u8* newHead);

struct MemoryArenaDebugData {
    ArenaDebugAllocate* debugAllocate;
    ArenaDebugDeallocate* debugDeallocate;
    void* data;
};





/////////////////
// Implementation

struct MemoryArena {
    // It would be possible to use a pointer to a function table containing both of the functions (more indirection but less memory requirement for main arena). Or do both in a single function which has a dispatch based on a parameter
    ArenaGrowFunc* grow;
    ArenaShrinkFunc* shrink;
    struct MemoryArenaDebugData* debugData; // 0 for non-debug arenas

    /*
     * There are many options for the selection of the arena parameters:
     * u8* memory, u64 pos, u64 capacity,  u64 commitPos
     * u8* memory, u64 size, u64 capacity, u64 commitPos
     * u8* head, u8* commitEnd, u8* capacityEnd (Would lose info about block base)
     */
    u8* memory;
    u64 pos;
    u64 commitPos;
    
    // This one is completely free for the grow and shrink functions to store data in. Normal arena operation does NOT touch this
    union {
        u64 additionalInteger;
        void* additionalPointer;
    };

#ifdef GROUNDED_ARENA_TEMP_MEMORY_CHECK
    u32 tempStackIndex;
#endif
};

// Default arena creation functions (More might be implemented by other modules or yourself)
GROUNDED_FUNCTION MemoryArena createFixedSizeArenaInBlock(u8* blockStart, u64 size);

GROUNDED_FUNCTION_INLINE void* _arenaPushSizeImpl(MemoryArena* arena, u64 size, u64 alignment, bool clear) {
    void* result = 0;
    u64 alignedPos = ALIGN_UP_POW2(arena->pos, alignment);
    ASSERT(alignedPos >= arena->pos);
    if(alignedPos + size > arena->commitPos) {
        // Grow
        u64 newSize = MAX(size, arena->commitPos);
        u8* newBlock = (u8*)arena->grow(arena, &newSize, alignment);
        if(newBlock == 0) return 0;
        if(newBlock == arena->memory + arena->commitPos) {
            // This is an in-place extension
            arena->commitPos += newSize;
        } else {
            // This is a completely new block
            arena->memory = newBlock;
            arena->pos = 0;
            arena->commitPos = newSize;
            alignedPos = 0;
        }
    }
    result = arena->memory + alignedPos;
    arena->pos = alignedPos + size;

    // This branch should get optimized out if clear is a compile time constant
    if(clear) { groundedClearMemory(result, size); }

    return result;
}

GROUNDED_FUNCTION_INLINE void* _arenaPushSize(MemoryArena* arena, u64 size, u64 alignment, bool clear, u64 line, String8 filename) {
    ASSERT(IS_POW2(alignment));
    ASSERT(alignment <= 4096);

    void* result = 0;

    // This branch should be very predictable in practice
    if(arena->debugData) {
        result = arena->debugData->debugAllocate(arena, size, alignment, clear, line, filename);
    } else {
        result = _arenaPushSizeImpl(arena, size, alignment, clear);
    }

    ASSERT(arena->pos <= arena->commitPos);

    // This is here so it can be enabled for all arenas with a single define
#ifdef GROUNDED_ARENA_OVERWRITE_WITH_SCRATCH
    if(!clear) { memset(result, 27, size); }
#endif

    return result;
}

GROUNDED_FUNCTION_INLINE void* _arenaPushCopy(MemoryArena* arena, u64 size, u64 alignment, void* original, u64 line, String8 filename) {
    void* result = 0;
    result = _arenaPushSize(arena, size, alignment, false, line, filename);
    if(result) {
        memcpy(result, original, size);
    }
    return result;
}

// Arena will release all memory after this function and should not be used anymore
GROUNDED_FUNCTION_INLINE void arenaRelease(MemoryArena* arena) {
    if(arena->memory) {
        arena->shrink(arena, 0);
    }
}

GROUNDED_FUNCTION_INLINE void _arenaPopTo(MemoryArena* arena, u8* newHead) {
    // Check if newHead is inside the current block
    if(newHead >= arena->memory && newHead - arena->memory <= (s64)arena->commitPos) {
        ASSERT(newHead <= arena->memory + arena->pos);
#ifdef GROUNDED_ARENA_OVERWRITE_WITH_SCRATCH
        memset(newHead, 27, (arena->memory + arena->pos) - newHead);
#endif
        arena->pos = newHead - arena->memory;
    } else {
        // Shrink
        arena->shrink(arena, newHead);
        ASSERT(arena->memory + arena->pos == newHead);
    }
}

GROUNDED_FUNCTION_INLINE void arenaPopTo(MemoryArena* arena, u8* newHead) {
    if(arena->debugData) {
            arena->debugData->debugDeallocate(arena, newHead);
    } else {
        _arenaPopTo(arena, newHead);
    }
}

GROUNDED_FUNCTION_INLINE MemoryArena* _arenaBootstrap(MemoryArena arena, u64 line, String8 filename) {
    MemoryArena* result = (MemoryArena*) _arenaPushSize(&arena, sizeof(MemoryArena), ALIGNMENT_OF(MemoryArena), false, line, filename);
    *result = arena;
    return result;
}

GROUNDED_FUNCTION_INLINE void* _arenaBootstrapPushSize(MemoryArena arena, u64 structSize, u64 offsetToArena, u64 alignment, u64 line, String8 filename) {
    ASSERT((offsetToArena & (ALIGNMENT_OF(MemoryArena)-1)) == 0);
    void* result = _arenaPushSize(&arena, structSize, alignment, true, line, filename);
    *(MemoryArena*)((u8*)result + offsetToArena) = arena;
    return result;
}


GROUNDED_FUNCTION_INLINE ArenaTempMemory arenaBeginTemp(MemoryArena* arena) {
    ArenaTempMemory result = {0};
    result.arena = arena;
    result.base = arena->memory;
    result.pos = arena->pos;
#ifdef GROUNDED_ARENA_TEMP_MEMORY_CHECK
    result.tempStackIndex = arena->tempStackIndex++;
#endif
    return result;
}
GROUNDED_FUNCTION_INLINE void arenaEndTemp(ArenaTempMemory temp) {
#ifdef GROUNDED_ARENA_TEMP_MEMORY_CHECK
    ASSERT(temp.tempStackIndex == --temp.arena->tempStackIndex);
#endif
    arenaPopTo(temp.arena, temp.base + temp.pos);
}

GROUNDED_FUNCTION_INLINE ArenaMarker arenaCreateMarker(MemoryArena* arena) {
    ArenaMarker result = {0};
    result.arena = arena;
    result.base = arena->memory;
    result.pos = arena->pos;
    return result;
}

GROUNDED_FUNCTION_INLINE void arenaResetToMarker(ArenaMarker marker) {
    arenaPopTo(marker.arena, marker.base + marker.pos);
}

#endif // GROUNDED_ARENA_H
