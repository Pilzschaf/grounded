#include <grounded/memory/grounded_arena.h>
#include <grounded/memory/grounded_memory.h>
#include <grounded/logger/grounded_logger.h>

#include <stdio.h>

GROUNDED_FUNCTION void* fixedSizeArenaGrow(MemoryArena* arena, u64* size, u64 alignment) {
    ASSERT(false);
    *size = 0;
    return 0;
}

GROUNDED_FUNCTION void fixedSizeArenaShrink(MemoryArena* arena, u8* newHead) {
    ASSERT(newHead == 0);
    if(newHead == 0) {
        MemorySubsystem* memorySubsystem = (MemorySubsystem*) arena->additionalPointer;
        memorySubsystem->release(memorySubsystem->context, arena->memory, arena->commitPos);
    }
}

GROUNDED_FUNCTION MemoryArena createFixedSizeArena(MemorySubsystem* memorySubsystem, u64 size) {
    MemoryArena result = {0};
    result.memory = (u8*)memorySubsystem->reserve(memorySubsystem->context, size);
    result.commitPos = size;
    result.grow = (ArenaGrowFunc*)&fixedSizeArenaGrow;
    result.shrink = (ArenaShrinkFunc*)&fixedSizeArenaShrink;
    result.additionalPointer = memorySubsystem;

    return result;
}


typedef struct MemoryBlockFooter {
    u8* prevBlockMemory;
    u64 prevBlockCommitPos;
    u64 prevBlockPos;
} MemoryBlockFooter;

GROUNDED_FUNCTION void* growingArenaGrow(MemoryArena* arena, u64* size, u64 alignment) {
    MemorySubsystem* memorySubsystem = osGetMemorySubsystem();
    alignment = MAX(ALIGNMENT_OF(MemoryBlockFooter), alignment);
    u64 blockSize = ALIGN_UP_POW2(MAX(*size, arena->additionalInteger), alignment) + sizeof(MemoryBlockFooter);
    // Align up to memory pages
    blockSize  = ALIGN_UP_POW2(blockSize, 4096);

    u8* result = (u8*)memorySubsystem->reserve(memorySubsystem->context, blockSize);
    if(!result) return 0;
    *size = blockSize - sizeof(MemoryBlockFooter);

    MemoryBlockFooter* footer = (MemoryBlockFooter*) (result + *size);
    footer->prevBlockMemory = arena->memory;
    footer->prevBlockCommitPos = arena->commitPos;
    footer->prevBlockPos = arena->pos;

    return result;
}

GROUNDED_FUNCTION void growingArenaShrink(MemoryArena* arena, u8* newHead) {
    MemorySubsystem* memorySubsystem = osGetMemorySubsystem();
    MemoryBlockFooter* footer = (MemoryBlockFooter*) (arena->memory + arena->commitPos);
    if(!newHead) {
        // Completely release the arena
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;
        while(memory) {
            footer = (MemoryBlockFooter*) (memory + commitPos);
            u8* prevMemory = footer->prevBlockMemory;
            u64 prevCommitPos = footer->prevBlockCommitPos;
            memorySubsystem->release(memorySubsystem->context, memory, commitPos + sizeof(MemoryBlockFooter));
            memory = prevMemory;
            commitPos = prevCommitPos;
        }
    } else {
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;

        // Should actually only be called if the arena must shrink
        ASSERT(newHead < memory || newHead > memory + commitPos);

        while(newHead < memory || newHead > memory + commitPos) {
            // We have to release a block
            footer = (MemoryBlockFooter*) (memory + commitPos);
            arena->memory = footer->prevBlockMemory;
            arena->commitPos = footer->prevBlockCommitPos;
            memorySubsystem->release(memorySubsystem->context, memory, commitPos+sizeof(MemoryBlockFooter));
            arena->pos = newHead - arena->memory;
            memory = arena->memory;
            commitPos = arena->commitPos;
        }
    }
}

//TODO: Make this multithreading safe
typedef struct DebugAllocationLogEntry {
    String8 filename;
    u64 line;
    u64 size;
    u8* base;
    struct DebugAllocationLogEntry* next;
} DebugAllocationLogEntry;
static DebugAllocationLogEntry freeAllocationLogSentinel;

GROUNDED_FUNCTION_INLINE DebugAllocationLogEntry* getFreeDebugAllocationLogEntry() {
    DebugAllocationLogEntry* result;
    if(freeAllocationLogSentinel.next) {
        result = freeAllocationLogSentinel.next;
        freeAllocationLogSentinel.next = result->next;
    } else {
        // We need to allocate a new block
        #define ENTRIES_PER_BLOCK 512
        DebugAllocationLogEntry* entries = (DebugAllocationLogEntry*) osAllocateMemory(sizeof(DebugAllocationLogEntry) * ENTRIES_PER_BLOCK);
        result = &entries[0];
        freeAllocationLogSentinel.next = &entries[1];
        for(u32 i = 1; i < ENTRIES_PER_BLOCK-1; ++i) {
            entries[i].next = &entries[i+1];
        }
    }
    MEMORY_CLEAR_STRUCT(result);
    return result;
}

GROUNDED_FUNCTION void* debugAllocateLog(MemoryArena* arena, u64 size, u64 alignment, bool clear, u64 line, String8 filename) {
    // Data pointer is the sentinel for the log entry linked list
    ASSERT(arena->debugData->data);
    DebugAllocationLogEntry* sentinel = (DebugAllocationLogEntry*) arena->debugData->data;

    u8* base = _arenaPushSizeImpl(arena, size, alignment, clear);

    DebugAllocationLogEntry* entry = getFreeDebugAllocationLogEntry();
    entry->filename = filename;
    entry->line = line;
    entry->size = size;
    entry->base = base;
    
    // Add to list of allocations
    entry->next = sentinel->next;
    sentinel->next = entry;

    printf("Allocate: at %.*s:%lu\t %lu bytes\n", (int)filename.size, filename.base, line, size);

    return base;
}

GROUNDED_FUNCTION void debugDeallocateLog(MemoryArena* arena, u8* newHead) {
    // Data pointer is the sentinel for the log entry linked list
    ASSERT(arena->debugData->data);
    DebugAllocationLogEntry* sentinel = (DebugAllocationLogEntry*) arena->debugData->data;

    // newHead might be inside a given allocation and it also might revert back many allocations!
    DebugAllocationLogEntry* entry = sentinel->next;
    while(entry) {
        if(newHead == entry->base) {
            // We exaclty release this allocation
            sentinel->next = entry->next;
            printf("Free allocation: at %.*s:%lu\t%lu bytes\n", (int)entry->filename.size, entry->filename.base, entry->line, entry->size);

            // Add entry to free list
            entry->next = freeAllocationLogSentinel.next;
            freeAllocationLogSentinel.next = entry;
            break;
        } else if(newHead > entry->base && newHead <= entry->base + entry->size) {
            // We set a head that is inside this allocation block. This effectively shrinks the size of this allocation
            if(entry->size != newHead - entry->base) {
                entry->size = newHead - entry->base;
                printf("Resize allocation: at %.*s:%lu\tto %lu bytes\n", (int)entry->filename.size, entry->filename.base, entry->line, entry->size);
            }
            break;
        } else {
            // newHead is not part of this allocation so we assume that this allocation is completely freed and newHead is part of an allocation before this
            DebugAllocationLogEntry* entryToRemove = entry;
            sentinel->next = entry->next;
            entry = entry->next;

            // Add entryToRemove to free list
            printf("Free allocation: at %.*s:%lu\t %" PRIu64 " bytes\n", (int)entryToRemove->filename.size, entryToRemove->filename.base, entryToRemove->line, entryToRemove->size);
            entryToRemove->next = freeAllocationLogSentinel.next;
            freeAllocationLogSentinel.next = entryToRemove;
        }
    }

    _arenaPopTo(arena, newHead);
}

//TODO: Static buffer for now
struct MemoryArenaDebugData dataElements[256];
u64 dataElementIndex;

GROUNDED_FUNCTION void enableDebugMemoryLoggingForArena(MemoryArena* arena) {
    ASSERT(!arena->debugData);
    ASSERT(dataElementIndex + 1 < ARRAY_COUNT(dataElements));
    struct MemoryArenaDebugData* data = &dataElements[dataElementIndex++];
    data->debugAllocate = debugAllocateLog;
    data->debugDeallocate = debugDeallocateLog;
    data->data = getFreeDebugAllocationLogEntry();

    arena->debugData = (struct MemoryArenaDebugData*) data;
}

// Arena Debug stuff
typedef struct MemoryBlockOverflowDetectHeader {
    u8* prevBlockMemory;
    u64 prevBlockCommitPos;
    u64 prevBlockPos;
    struct MemoryBlockOverflowDetectHeader* prevHeader;
} MemoryBlockOverflowDetectHeader;

GROUNDED_FUNCTION void* debugAllocateOverflowDetect(MemoryArena* arena, u64 size, u64 alignment, bool clear, u64 line, String8 filename) {
    u64 dataAlignment = alignment;
    u64 requiredBlockAlignment = ALIGNMENT_OF(MemoryBlockOverflowDetectHeader);
    // Alignment of external allocation
    u64 dataStart = ALIGN_UP_POW2(sizeof(MemoryBlockOverflowDetectHeader), dataAlignment);
    u64 blockSize = dataStart + size;

    u8* result = osAllocateGuardedMemory(blockSize, requiredBlockAlignment, true);
    ASSERT(result);

    MemoryBlockOverflowDetectHeader* header = (MemoryBlockOverflowDetectHeader*) (result);
    header->prevBlockMemory = arena->memory;
    header->prevBlockCommitPos = arena->commitPos;
    header->prevBlockPos = arena->pos;
    header->prevHeader = arena->additionalPointer;
    arena->additionalPointer = header;

    u64 newSize = blockSize - dataStart;
    u8* newBlock = result + dataStart;

    // This is always a new block
    arena->memory = newBlock;
    arena->pos = 0;
    arena->commitPos = newSize;
    arena->pos = size;

    if(!clear) { memset(newBlock, 27, newSize); }

    return arena->memory;
}

GROUNDED_FUNCTION void debugDeallocateOverflowDetect(MemoryArena* arena, u8* newHead) {
    MemoryBlockOverflowDetectHeader* header = (MemoryBlockOverflowDetectHeader*) (arena->additionalPointer);
    if(!newHead) {
        // Completely release the arena
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;
        while(header) {
            u8* prevMemory = header->prevBlockMemory;
            u64 prevCommitPos = header->prevBlockCommitPos;
            header = header->prevHeader;
            osFreeGuardedMemory(memory, commitPos, true);
            memory = prevMemory;
            commitPos = prevCommitPos;
        }
        osFreeGuardedMemory(memory, commitPos, true);
    } else {
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;

        if(newHead == memory + arena->pos) {
            // We should reset to the same point we already are so ignore...
        } else {
            // Should actually only be called if the arena must shrink.
            //TODO: This is not true anymore as allocation is always done in whole pages but an application might release to the middle of a page.
            ASSERT(newHead < memory || newHead > memory + commitPos);
            while(newHead < memory || newHead > memory + commitPos) {
                // We have to release a block
                arena->memory = header->prevBlockMemory;
                arena->commitPos = header->prevBlockCommitPos;
                header = header->prevHeader;
                osFreeGuardedMemory(memory-sizeof(MemoryBlockOverflowDetectHeader), commitPos+sizeof(MemoryBlockOverflowDetectHeader), true);
                arena->pos = newHead - arena->memory;
                memory = arena->memory;
                commitPos = arena->commitPos;
            }
            arena->additionalPointer = header;
        }
    }
}

static struct MemoryArenaDebugData overflowDetectDebugFunctionTable = {
    .debugAllocate = debugAllocateOverflowDetect,
    .debugDeallocate = debugDeallocateOverflowDetect,
};

GROUNDED_FUNCTION void enableDebugMemoryOverflowDetectForArena(MemoryArena* arena) {
    ASSERT(!arena->debugData);
    arena->debugData = &overflowDetectDebugFunctionTable;
}

GROUNDED_FUNCTION void* debugAllocateUndeflowDetect(MemoryArena* arena, u64 size, u64 alignment, bool clear, u64 line, String8 filename) {
    alignment = MAX(ALIGNMENT_OF(MemoryBlockFooter), alignment);
    u64 blockSize = ALIGN_UP_POW2(size, ALIGNMENT_OF(MemoryBlockFooter)) + sizeof(MemoryBlockFooter);

    u8* result = osAllocateGuardedMemory(blockSize, alignment, false);
    ASSERT(result);
    u64 usableSize = blockSize - sizeof(MemoryBlockFooter);

    MemoryBlockFooter* footer = (MemoryBlockFooter*) (result + usableSize);
    footer->prevBlockMemory = arena->memory;
    footer->prevBlockCommitPos = arena->commitPos;
    footer->prevBlockPos = arena->pos;

    // This is always a new block
    arena->memory = result;
    arena->commitPos = usableSize;
    arena->pos = size;

    if(!clear) { memset(result, 27, usableSize); }

    return arena->memory;
}

GROUNDED_FUNCTION void debugDeallocateUnderflowDetect(MemoryArena* arena, u8* newHead) {
    MemoryBlockFooter* footer = (MemoryBlockFooter*) (arena->memory + arena->commitPos);
    if(!newHead) {
        // Completely release the arena
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;
        while(memory) {
            footer = (MemoryBlockFooter*) (memory + commitPos);
            u8* prevMemory = footer->prevBlockMemory;
            u64 prevCommitPos = footer->prevBlockCommitPos;
            osFreeGuardedMemory(memory, commitPos, false);
            memory = prevMemory;
            commitPos = prevCommitPos;
        }
    } else {
        u8* memory = arena->memory;
        u64 commitPos = arena->commitPos;

        // Should actually only be called if the arena must shrink
        ASSERT(newHead < memory || newHead > memory + commitPos);
        ASSERT(memory);

        while(newHead < memory || newHead > memory + commitPos) {
            // We have to release a block
            footer = (MemoryBlockFooter*) (memory + commitPos);
            arena->memory = footer->prevBlockMemory;
            arena->commitPos = footer->prevBlockCommitPos;
            osFreeGuardedMemory(memory, commitPos, false);
            arena->pos = newHead - arena->memory;
            ASSERT(arena->memory);
            memory = arena->memory;
            commitPos = arena->commitPos;
        }
    }
}

static struct MemoryArenaDebugData underflowDetectDebugFunctionTable = {
    .debugAllocate = debugAllocateUndeflowDetect,
    .debugDeallocate = debugDeallocateUnderflowDetect,
};

GROUNDED_FUNCTION void enableDebugMemoryUnderflowDetectForArena(MemoryArena* arena) {
    ASSERT(!arena->debugData);
    arena->debugData = &underflowDetectDebugFunctionTable;
}
// End Arena Debug stuff





GROUNDED_FUNCTION MemoryArena createGrowingArena(MemorySubsystem* memorySubsystem, u64 minBlockSize) {
    MemoryArena result = {0};
    // Round up to page size
    u64 size = ALIGN_UP_POW2(minBlockSize, 4096);
    result.memory = (u8*)memorySubsystem->reserve(memorySubsystem->context, size);
    result.commitPos = size - sizeof(MemoryBlockFooter);

    result.grow = (ArenaGrowFunc*)&growingArenaGrow;
    result.shrink = (ArenaShrinkFunc*)&growingArenaShrink;
    result.additionalInteger = minBlockSize;

    MemoryBlockFooter* footer = (MemoryBlockFooter*) (result.memory + result.commitPos);
    footer->prevBlockMemory = 0;
    footer->prevBlockCommitPos = 0;
    footer->prevBlockPos = 0;
    
    return result;
}
