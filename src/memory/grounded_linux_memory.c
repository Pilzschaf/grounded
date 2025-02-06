#include <grounded/memory/grounded_memory.h>

#include <sys/mman.h>
#include <unistd.h>

#include <stdlib.h> // For strtoul, strtof
#include <ctype.h> // For isspace

GROUNDED_FUNCTION void* osAllocateMemory(u64 size) {
    void* memory = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //void* memory = (void*)__syscall_ret(__NR_mmap, 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    // mmap actually returns -1 on failure so we translate this to a null pointer
    if(memory == (void *) -1) {
        memory = 0;
    }
    
    return memory;
}

// Reserve but do not commit yet
GROUNDED_FUNCTION void* osReserveMemory(u64 size) {
    void* memory = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    // mmap actually returns -1 on failure so we translate this to a null pointer
    if(memory == (void *) -1) {
        memory = 0;
    }

    return memory;
}

//TODO: Do not assume page size and use u64 pageSize = getpagesize(); instead

GROUNDED_FUNCTION void* osAllocateGuardedMemory(u64 size, u64 alignment, bool overflow) {
    u64 allocateSize = ALIGN_UP_POW2(size, 4096) + 4096;
    u8* memory = (u8*)mmap(0, allocateSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(overflow) {
        mprotect(memory + allocateSize - 4096, 4096, PROT_NONE);
        u64 offset = ALIGN_DOWN_POW2((allocateSize - 4096) - size, alignment);
        return memory + offset;
    } else {
        mprotect(memory, 4096, PROT_NONE);
        return memory + 4096;
    }
}

GROUNDED_FUNCTION void osFreeGuardedMemory(void* memory, u64 size, bool overflow) {
    u64 allocateSize = ALIGN_UP_POW2(size, 4096) + 4096;
    if(overflow) {
        munmap(PTR_FROM_INT(ALIGN_DOWN_POW2(INT_FROM_PTR(memory), 4096)), allocateSize);
    } else {
        munmap(((u8*)memory) - 4096, allocateSize);
    }
}

GROUNDED_FUNCTION void osCommitMemory(void* memory, u64 size) {
    mprotect(memory, size, PROT_READ | PROT_WRITE);
}
GROUNDED_FUNCTION void osDecommitMemory(void* memory, u64 size) {
    mprotect(memory, size, PROT_NONE);
}
GROUNDED_FUNCTION void osReleaseMemory(void* memory, u64 size) {
    munmap(memory, size);
}


GROUNDED_FUNCTION void* osReserve(void* context, u64 size) {
    return osAllocateMemory(size);
}

GROUNDED_FUNCTION void osCommit(void* context, void* ptr, u64 size) {
    osCommitMemory(ptr, size);
}

GROUNDED_FUNCTION void osDecommit(void* context, void* ptr, u64 size) {
    osDecommitMemory(ptr, size);
}

GROUNDED_FUNCTION void osRelease(void* context, void* ptr, u64 size) {
    osReleaseMemory(ptr, size);
}



GROUNDED_FUNCTION void* contigousVirtualMemoryArenaGrow(MemoryArena* arena, u64* size, u64 alignment) {
    ASSERT(arena->pos + *size > arena->commitPos);
    u64 commitSize = ALIGN_UP_POW2(*size, 4096);
    osCommitMemory(arena->memory + arena->commitPos, commitSize);
    *size = commitSize;
    return arena->memory + arena->commitPos;
}

GROUNDED_FUNCTION void contigousVirtualMemoryArenaShrink(MemoryArena* arena, u8* newHead) {
    ASSERT(newHead == 0);
    if(newHead == 0) {
        // Decommit not necessary
        //osDecommitMemory(arena->memory, arena->commitPos);
        osReleaseMemory(arena->memory, arena->additionalInteger);
    }
}

GROUNDED_FUNCTION MemoryArena createContigousVirtualMemoryArena(u64 maxVirtualMemorySize) {
    ASSERT(sizeof(void*) >= 8);
    MemoryArena result = {};
    result.memory = osReserveMemory(maxVirtualMemorySize);
    result.additionalInteger = maxVirtualMemorySize;
    result.grow = (ArenaGrowFunc*) &contigousVirtualMemoryArenaGrow;
    result.shrink = (ArenaShrinkFunc*) &contigousVirtualMemoryArenaShrink;
    return result;
}


GROUNDED_FUNCTION MemorySubsystem* osGetMemorySubsystem() {
    static MemorySubsystem memorySubsystem = {};
    if(memorySubsystem.reserve == 0) {
        memorySubsystem.reserve = osReserve;
        memorySubsystem.commit = osCommit;
        memorySubsystem.decommit = osDecommit;
        memorySubsystem.release = osRelease;
        memorySubsystem.allowsSeparateCommit = true;
    }
    return &memorySubsystem;
}


GROUNDED_FUNCTION GroundedCircularBuffer groundedCreateCircularBuffer(u64 minimumSize) {
    GroundedCircularBuffer result = {};
    u64 pageSize = getpagesize();
    ASSERT(IS_POW2(pageSize));
    if(IS_POW2(pageSize)) {
        result.size = ALIGN_UP_POW2(minimumSize, pageSize);
        result.fd = memfd_create("circular_buffer", 0);
        ftruncate(result.fd, result.size);

        // Address location we want to submit the copies to
        result.buffer = mmap(0, result.size*2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        // Map buffer at the start
        mmap(result.buffer, result.size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, result.fd, 0);
        // Map again after first mapping
        mmap(result.buffer + result.size, result.size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, result.fd, 0);
    }
    return result;
}

GROUNDED_FUNCTION void groundedDestroyCircularBuffer(GroundedCircularBuffer* circularBuffer) {
    munmap(circularBuffer->buffer + circularBuffer->size, circularBuffer->size);
    munmap(circularBuffer->buffer, circularBuffer->size);
    close(circularBuffer->fd);
    circularBuffer->fd = 0;
    circularBuffer->buffer = 0;
    circularBuffer->size = 0;
}

#include "grounded_stream.inl"