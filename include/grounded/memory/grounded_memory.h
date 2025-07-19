#ifndef GROUNDED_MEMORY_H
#define GROUNDED_MEMORY_H

#include "../grounded.h"

#include <string.h> // Required for memset, memcmp, memcpy

#define MEMORY_CLEAR(p,z) groundedClearMemory((p), (z))
#define MEMORY_CLEAR_STRUCT(p) MEMORY_CLEAR((p), sizeof(*(p)))
#define MEMORY_CLEAR_ARRAY(p) MEMORY_CLEAR((p), sizeof(p))
#define MEMORY_CLEAR_TYPED(p,c) MEMORY_CLEAR((p), sizeof(*(p))*(c))

// Does not handle overlapping source and dest
#define MEMORY_COPY(d,s,z) groundedCopyMemory((d),(s),(z))
#define MEMORY_COPY_STRUCT(d,s) MEMORY_COPY((d),(s),(CLAMP_TOP(sizeof(*(d)), sizeof(*(s)))))
#define MEMORY_COPY_ARRAY(d,s) MEMORY_COPY((d),(s),CLAMP_TOP(sizeof(s),sizeof(d)))
#define MEMORY_COPY_TYPED(d,s,c) MEMORY_COPY((d),(s),CLAMP_TOP(sizeof(*(d)),sizeof(*(s)))*(c))

#define MEMORY_IS_ZERO(d, s) (groundedMemoryIsZero(d, s))

// Clears memory at ptr with the specified size to 0
GROUNDED_FUNCTION_INLINE void groundedClearMemory(void* ptr, u64 size) {
    ASSERT(ptr);
    #ifdef GROUNDED_NO_STDLIB
    u8* b = (u8*)ptr;
    while(size--) {
        *b++ = 0;
    }
    #else
    memset(ptr, 0, size);
    #endif
}

// Sets size bytes from ptr onwards to value
GROUNDED_FUNCTION_INLINE void groundedSetMemory(void* ptr, u8 value, u64 size) {
    ASSERT(ptr);
    #ifdef GROUNDED_NO_STDLIB
    u8* b = (u8*)ptr;
    while (size--) {
        *b++ = value;
    }
    #else
    memset(ptr, value, size);
    #endif
}

// Returns false if they don't match
GROUNDED_FUNCTION_INLINE bool groundedCompareMemory(void* a, void* b, u64 size) {
    // Spec does not allow passing null pointers to memcmp
    ASSERT(a && b);
    #ifdef GROUNDED_NO_STDLIB
    u8* ptra = (u8*)a;
    u8* ptrb = (u8*)b;
    while(size--) {
        if(*ptra++ != *ptrb++) {
            return false;
        }
    }
    return true;
    #else
    return memcmp(a, b, size) == 0;
    #endif
}

GROUNDED_FUNCTION_INLINE void groundedCopyMemory(void* dest, void* src, u64 size) {
    ASSERT(src);
    ASSERT(dest);
    #ifdef GROUNDED_NO_STDLIB
    u8* srcPtr = (u8*)src;
    u8* dstPtr = (u8*)dest;
    while(size--) {
        *dstPtr++ = *srcPtr++;
    }
    #else
    memcpy(dest, src, size);
    #endif
}

GROUNDED_FUNCTION_INLINE bool groundedMemoryIsZero(void* memory, u64 size) {
    u8* data = (u8*)memory;
    for(u64 i = 0; i < size; ++i) {
        if(data[i] != 0) {
            return false;
        }
    }
    return true;
}

#include "grounded_arena.h"

///////////////
// Memory stuff
typedef void* MemoryReserveFunc(void* context, u64 size);
typedef void MemoryCommitFunc(void* context, void* ptr, u64 size);
typedef void MemoryDecommitFunc(void* context, void* ptr, u64 size);
typedef void MemoryReleaseFunc(void* context, void* ptr, u64 size);

typedef struct {
    MemoryReserveFunc* reserve;
    MemoryCommitFunc* commit;
    MemoryDecommitFunc* decommit;
    MemoryReleaseFunc* release;
    void* context;
    bool allowsSeparateCommit;
} MemorySubsystem;

GROUNDED_FUNCTION void* osAllocateMemory(u64 size);
GROUNDED_FUNCTION void* osReserveMemory(u64 size);
GROUNDED_FUNCTION void* osAllocateGuardedMemory(u64 size, u64 alignment, bool overflow);
GROUNDED_FUNCTION void osFreeGuardedMemory(void* memory, u64 size, bool overflow);

GROUNDED_FUNCTION MemorySubsystem* osGetMemorySubsystem();
GROUNDED_FUNCTION MemorySubsystem* getMallocMemorySubsystem();

// If Overflow or Underflow protection is enabled fixed size behaves like a growing arena
GROUNDED_FUNCTION struct MemoryArena createFixedSizeArena(MemorySubsystem* memorySubsystem, u64 size);
GROUNDED_FUNCTION struct MemoryArena createGrowingArena(MemorySubsystem* memorySubsystem, u64 minBlockSize);
GROUNDED_FUNCTION struct MemoryArena createContigousVirtualMemoryArena(u64 maxVirtualMemorySize);

// Circular Buffer aka. Ring buffer
typedef struct {
    u8* buffer;
    u64 size;
    union {
        int fd;
        void* handle;
    };
} GroundedCircularBuffer;

GROUNDED_FUNCTION GroundedCircularBuffer groundedCreateCircularBuffer(u64 minimumSize); // Minimum size is scaled up to pagesize
GROUNDED_FUNCTION void groundedDestroyCircularBuffer(GroundedCircularBuffer* circularBuffer);

// Helper functions which make the usage of the circular buffer easier
typedef struct  {
    GroundedCircularBuffer buffer;
    u8* writeHead;
    u8* readHead;
} GroundedCircularBufferReadWriter;

GROUNDED_FUNCTION_INLINE GroundedCircularBufferReadWriter groundedCreateCircularBufferReadWriter(u64 minimumSize) {
    GroundedCircularBufferReadWriter result = {
        .buffer = groundedCreateCircularBuffer(minimumSize),
    };
    result.readHead = result.buffer.buffer;
    result.writeHead = result.buffer.buffer;
    return result;
}

GROUNDED_FUNCTION_INLINE u64 spaceLeftToWrite(GroundedCircularBufferReadWriter* circularBuffer) {
    u64 result = 0;
    if(circularBuffer->writeHead < circularBuffer->readHead) {
        // -1 because it is not allowed to write to the same location as the read head as this would mean an empty buffer
        result = (circularBuffer->readHead - circularBuffer->writeHead) - 1;
    } else {
        // -1 because it is not allowed to write to the same location as the read head as this would mean an empty buffer
        result = (circularBuffer->buffer.size - (circularBuffer->writeHead - circularBuffer->readHead)) - 1;
    }
    return result;
}

GROUNDED_FUNCTION_INLINE void fixupWriteHead(GroundedCircularBufferReadWriter* circularBuffer) {
    u64 offset = circularBuffer->writeHead - circularBuffer->buffer.buffer;
    if(offset >= circularBuffer->buffer.size) {
        offset -= circularBuffer->buffer.size;
        circularBuffer->writeHead = circularBuffer->buffer.buffer + offset;
    }
}

GROUNDED_FUNCTION_INLINE void fixupReadHead(GroundedCircularBufferReadWriter* circularBuffer) {
    u64 offset = circularBuffer->readHead - circularBuffer->buffer.buffer;
    if(offset >= circularBuffer->buffer.size) {
        offset -= circularBuffer->buffer.size;
        circularBuffer->readHead = circularBuffer->buffer.buffer + offset;
    }
}

GROUNDED_FUNCTION_INLINE void fixupHeads(GroundedCircularBufferReadWriter* circularBuffer) {
    fixupWriteHead(circularBuffer);
    fixupReadHead(circularBuffer);
}

// Returns 0 if write was not possible. Otherwise it returns the location where the data has been placed in the circular buffer
GROUNDED_FUNCTION_INLINE u8* writeToCircularBuffer(GroundedCircularBufferReadWriter* circularBuffer, u8* data, u64 size) {
    u8* result = 0;
    // Can not store a single element that is >= the size of the total buffer
    ASSERT(size < circularBuffer->buffer.size);

    if(spaceLeftToWrite(circularBuffer) >= size) {
        //TODO: Alignment
        result = circularBuffer->writeHead;
        memcpy(result, data, size);

        circularBuffer->writeHead += size;
        fixupWriteHead(circularBuffer);
    }
    return result;
}

GROUNDED_FUNCTION_INLINE u64 spaceLeftToRead(GroundedCircularBufferReadWriter* circularBuffer) {
    u64 result = 0;
    if(circularBuffer->readHead <= circularBuffer->writeHead) {
        result = circularBuffer->writeHead - circularBuffer->readHead;
    } else {
        result = circularBuffer->buffer.size - (circularBuffer->readHead - circularBuffer->writeHead);
    }
    return result;
}

// Returns 0 if read was not possible
GROUNDED_FUNCTION_INLINE u8* readFromCircularBuffer(GroundedCircularBufferReadWriter* circularBuffer, u64 size) {
    u8* result = 0;
    // Can not read a single element that is >= size of the total buffer
    ASSERT(size < circularBuffer->buffer.size);
    if(spaceLeftToRead(circularBuffer) >= size) {
        result = circularBuffer->readHead;
        
        circularBuffer->readHead += size;
        fixupReadHead(circularBuffer);
    }
    return result;
}

#endif // GROUNDED_MEMORY_H
