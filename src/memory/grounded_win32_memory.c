#include <grounded/memory/grounded_memory.h>
#include <grounded/memory/grounded_arena.h>
#include <grounded/memory/grounded_stream.h>

#include <windows.h>

GROUNDED_FUNCTION void* osAllocateMemory(u64 size) {
    void* memory = VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
    
    return memory;
}

// Reserve but do not commit yet
GROUNDED_FUNCTION void* osReserveMemory(u64 size) {
    void* memory = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);

    return memory;
}

GROUNDED_FUNCTION void* osAllocateGuardedMemory(u64 size, u64 alignment, bool overflow) {
    u64 allocateSize = ALIGN_UP_POW2(size, 4096) + 4096;
    u8* memory = (u8*)VirtualAlloc(0, allocateSize, MEM_COMMIT, PAGE_READWRITE);
    if(overflow) {
        VirtualProtect(memory + allocateSize - 4096, 4096, PAGE_NOACCESS, 0);
        u64 offset = ALIGN_DOWN_POW2((allocateSize - 4096) - size, alignment);
        return memory + offset;
    } else {
        DWORD oldProtect;
        VirtualProtect(memory, 4096, PAGE_NOACCESS, &oldProtect);
        return memory + 4096;
    }
}

GROUNDED_FUNCTION void osFreeGuardedMemory(void* memory, u64 size, bool overflow) {
    u64 allocateSize = ALIGN_UP_POW2(size, 4096) + 4096;
    if(overflow) {
        VirtualFree(PTR_FROM_INT(ALIGN_DOWN_POW2(INT_FROM_PTR(memory), 4096)), 0, MEM_RELEASE);
    } else {
        VirtualFree(((u8*)memory) - 4096, 0, MEM_RELEASE);
    }
}

GROUNDED_FUNCTION void osCommitMemory(void* memory, u64 size) {
    VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
}
GROUNDED_FUNCTION void osDecommitMemory(void* memory, u64 size) {
    VirtualFree(memory, size, MEM_DECOMMIT);
}
GROUNDED_FUNCTION void osReleaseMemory(void* memory, u64 size) {
    VirtualFree(memory, 0, MEM_RELEASE);
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
    MemoryArena result = {0};
    result.memory = osReserveMemory(maxVirtualMemorySize);
    result.additionalInteger = maxVirtualMemorySize;
    result.grow = (ArenaGrowFunc*) &contigousVirtualMemoryArenaGrow;
    result.shrink = (ArenaShrinkFunc*) &contigousVirtualMemoryArenaShrink;
    return result;
}


GROUNDED_FUNCTION MemorySubsystem* osGetMemorySubsystem() {
    static MemorySubsystem memorySubsystem = {0};
    if(memorySubsystem.reserve == 0) {
        memorySubsystem.reserve = osReserve;
        memorySubsystem.commit = osCommit;
        memorySubsystem.decommit = osDecommit;
        memorySubsystem.release = osRelease;
        memorySubsystem.allowsSeparateCommit = false; //TODO: Allow
    }
    return &memorySubsystem;
}

GROUNDED_FUNCTION GroundedCircularBuffer groundedCreateCircularBuffer(u64 minimumSize) {
    GroundedCircularBuffer result = {0};
    // Seems like windows requires at least 64k
    u64 pageSize = 0x10000;
    ASSERT(IS_POW2(pageSize));
    if(IS_POW2(pageSize)) {
        bool success = false;
        u8* basePointer = 0;
        void* desiredAddress = 0;
        while(!success) {
            success = true;
            result.size = ALIGN_UP_POW2(minimumSize, pageSize);
            u64 allocSize = result.size * 2;
            
            // Try to find a viable address
            desiredAddress = VirtualAlloc(0, allocSize, MEM_RESERVE, PAGE_NOACCESS);
            VirtualFree(desiredAddress, 0, MEM_RELEASE);

            result.handle = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, allocSize >> 32, allocSize & 0xffffffffu, 0);
            if(result.handle == INVALID_HANDLE_VALUE) success = false;
            if(success) {
                basePointer = (u8*)MapViewOfFileEx(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, result.size, desiredAddress);
            }
            if(success) {
                MapViewOfFileEx(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, result.size, ((u8*)desiredAddress) + result.size);
            }

            if(!success) {
                groundedDestroyCircularBuffer(&result);
            }
        }
        result.buffer = basePointer;
    }
    return result;
}

GROUNDED_FUNCTION void groundedDestroyCircularBuffer(GroundedCircularBuffer* circularBuffer) {
    if(circularBuffer->buffer) {
        UnmapViewOfFile(circularBuffer->buffer);
        UnmapViewOfFile(circularBuffer->buffer + circularBuffer->size);
    }
    if(circularBuffer->handle) {
        CloseHandle(circularBuffer->handle);
    }
    circularBuffer->handle = 0;
    circularBuffer->buffer = 0;
    circularBuffer->size = 0;
}

#include "grounded_stream.inl"
