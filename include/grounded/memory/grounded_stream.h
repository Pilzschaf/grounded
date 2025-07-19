#ifndef GROUNDED_STREAM_H
#define GROUNDED_STREAM_H

#include <grounded/string/grounded_string.h>
#include <grounded/memory/grounded_memory.h>

enum GroundedStreamErrorCode {
    GROUNDED_STREAM_SUCCESS,
    GROUNDED_STREAM_PAST_EOF,
    GROUNDED_STREAM_IO_ERROR,
    GROUNDED_STREAM_CORRUPTED_DATA,
    GROUNDED_STREAM_ERROR_COUNT,
};

// https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/
// This basically abstracts stream IO from file/memory specifics
// Functions can simply expect a BufferedStreamReader and Caller can handle data from memory, file, etc.
// The setup allows for unintrusive handling of escape characters, decompression, decryption etc.
// An application would usually expect a BufferedStreamReader/Writer as input to its function but instead of direclty using it
// The recommended way is to use some helper constructs like SimpleReader/Writer or TextualReader/Writer depending on requirements

/////////////////////////
// Buffered Stream Reader
typedef struct BufferedStreamReader {
    // Buffer should not be writtern to by the application
    const u8* start; // Start of the buffer. Should only be used by the implementation
    const u8* end; // opl
    const u8* cursor; // The current read cursor
    void* implementationPointer; // Only for use by the implementation
    enum GroundedStreamErrorCode error; // Persisting first error code or success if no error occured
    enum GroundedStreamErrorCode (*refill)(struct BufferedStreamReader* r); // Call this function once cursor == end. This will make a new buffer available. After this cursor will point to the start of the new data and it must hold that cursor < end. Or in other words there must be at least a single new byte of data.
    void (*close)(struct BufferedStreamReader* r);  // Call this after you finished reading. BufferedStreamReader must not be used after calling this function!
} BufferedStreamReader;

GROUNDED_FUNCTION_INLINE void dummyBufferedStreamReaderClose(struct BufferedStreamReader* r) {};

GROUNDED_FUNCTION_INLINE enum GroundedStreamErrorCode refillZeros(BufferedStreamReader* r) {
    static const u8 zeros[256] = {0};
    r->start = zeros;
    r->cursor = zeros;
    r->end = zeros + sizeof(zeros);
    return r->error;
}

GROUNDED_FUNCTION_INLINE enum GroundedStreamErrorCode refillMemStream(BufferedStreamReader* r) {
    r->error = GROUNDED_STREAM_PAST_EOF;
    r->refill = refillZeros;
    return r->refill(r);
}

GROUNDED_FUNCTION_INLINE BufferedStreamReader createMemoryStreamReader(u8* buffer, u64 size) {
    BufferedStreamReader result = {
        .start = buffer,
        .end = buffer + size,
        .cursor = buffer,
        .implementationPointer = buffer,
        .error = GROUNDED_STREAM_SUCCESS,
        .refill = refillMemStream,
        .close = dummyBufferedStreamReaderClose,
    };
    return result;
}


/////////////////////////
// Buffered Stream Writer

typedef struct BufferedStreamWriter {
    u8* start; // Current position
    u8* end; // opl
    void* implementationPointer; // Only for use by the implementation
    enum GroundedStreamErrorCode error;
    u32 flags;
    enum GroundedStreamErrorCode (*submit)(struct BufferedStreamWriter* w, u8* opl);
    // Call this once all data has been written (performs cleanup tasks if they are required like closing file handles)
    void (*close)(struct BufferedStreamWriter* w);
} BufferedStreamWriter;

GROUNDED_FUNCTION_INLINE enum GroundedStreamErrorCode submitScratch(BufferedStreamWriter* w, u8* opl) {
    // TODO: Scratch is thread-local to circumvent thread cache thrashing
    static u8 scratch[256] = {0};
    //static u8 scratch[256];
    //_Thread_local int test;
    w->start = scratch;
    w->end = scratch + sizeof(scratch);
    if(w->error == GROUNDED_STREAM_SUCCESS) {
        w->error = GROUNDED_STREAM_PAST_EOF;
    }
    return w->error;
}

GROUNDED_FUNCTION_INLINE BufferedStreamWriter createMemoryStreamWriter(u8* buffer, u64 size) {
    BufferedStreamWriter result = {
        .start = buffer,
        .end = buffer + size,
        .implementationPointer = buffer,
        .error = GROUNDED_STREAM_SUCCESS,
        .submit = submitScratch, // submit is always an error case for us as we exceed buffer
    };
    return result;
}

GROUNDED_FUNCTION_INLINE void memoryStreamWriterClose(BufferedStreamWriter* w) {
    ASSUME(w) {
        w->submit(w, w->start);
        if(w->close) {
            w->close(w);
        }
        w->submit = submitScratch;
    }
}









////////////////
// Simple Reader

// Meant for reading binary data
typedef struct SimpleReader {
    BufferedStreamReader r;
    u64 bytesRead;
} SimpleReader;

#define SIMPLE_READER_READ(reader, target) (simpleReaderRead(reader, target, sizeof(*target)))
#define SIMPLE_READER_READ_ARRAY(reader, count, target) (simpleReaderRead(reader, target, sizeof(*target) * count))

GROUNDED_FUNCTION_INLINE SimpleReader createSimpleReader(BufferedStreamReader reader) {
    SimpleReader result = {
        .r = reader,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE void simpleReaderRead(SimpleReader* reader, const void* data, u64 size) {
    u8* dst = (u8*)data;
    s64 sizeLeft = reader->r.end - reader->r.cursor;
    while(sizeLeft < (s64)size) {
        memcpy(dst, reader->r.cursor, sizeLeft);
        reader->bytesRead += sizeLeft;
        reader->r.refill(&reader->r);
        size -= sizeLeft;
        dst = dst + sizeLeft;
        sizeLeft = reader->r.end - reader->r.cursor;
    }
    memcpy(dst, reader->r.cursor, size);
    reader->r.cursor += size;
    reader->bytesRead += size;
}

// This is a bit harder as we do not know how large the buffer should be and cannot expect subsequent allocation to be directly after each other.
// Best option is probably to ping pong back and forth between both arenas, always dubling buffer size until large enough
GROUNDED_FUNCTION_INLINE u8* simpleReaderReadUntilDelimeter(SimpleReader* reader, MemoryArena* arena, u32 delimeterCount, const char* delimeters, u64* dataSize) {
    // This code has never been tested and probably contains bugs
    ASSERT(false);
    
    MemoryArena* scratch = threadContextGetScratch(arena);

    MemoryArena* arenas[2] = {arena, scratch};
    u32 currentArenaIndex = 0;
    u8* buffer = 0;
    ArenaMarker marker = arenaCreateMarker(arena);

    u64 bufferSize = KB(4);
    u64 currentDataSize = 0;
    buffer = ARENA_PUSH_ARRAY(arenas[currentArenaIndex], bufferSize, u8);

    while(true) {
        u8* dst = buffer + currentDataSize;
        if(reader->r.cursor >= reader->r.end) {
            reader->r.refill(&reader->r);
        }
        u8* cursor = (u8*)reader->r.cursor;

        bool delimeter = false;
        for(u32 i = 0; i < delimeterCount; ++i) {
            if(delimeters[i] == *cursor) {
                delimeter = true;
                break;
            }
        }

        if(delimeter) {
            // Check if this arena is the correct one. Otherwise we must do a copy into the correct one
            if(currentArenaIndex != 0) {
                u8* finalBuffer = ARENA_PUSH_ARRAY(arena, currentDataSize, u8);
                memcpy(finalBuffer, buffer, currentDataSize);
                arenaResetToMarker(marker);
                buffer = finalBuffer;
            } else {
                // Shrink arena back down to required size
                arenaPopTo(arenas[currentArenaIndex], buffer + currentDataSize);
            }
            break;
        } else {
            *dst = *cursor;
            reader->r.cursor++;
            ++currentDataSize;
            ++reader->bytesRead;

            // Data size exceeds buffer size. Double buffer size and flip buffers
            if(currentDataSize >= bufferSize) {
                bufferSize *= 2;
                u32 nextArenaIndex = (currentArenaIndex + 1) % 2;
                ArenaMarker nextMarker = arenaCreateMarker(arenas[nextArenaIndex]);
                u8* nextBuffer = ARENA_PUSH_ARRAY(arenas[nextArenaIndex], bufferSize, u8);
                memcpy(nextBuffer, buffer, currentDataSize);
                arenaResetToMarker(marker);
                currentArenaIndex = nextArenaIndex;
                buffer = nextBuffer;
                marker = nextMarker;
            }
        }
    }

    *dataSize = currentDataSize;
    return buffer;
}


/////////////////
// Textual Reader

// Meant to read in textual data
typedef struct TextualReader {
    BufferedStreamReader r;
} TextualReader;

GROUNDED_FUNCTION_INLINE TextualReader createTextualReader(BufferedStreamReader r) {
    TextualReader result = {
        .r = r,
    };
    return result;
}

GROUNDED_FUNCTION u64 textualReaderReadUntilWhitespace(TextualReader* reader, u8* buffer, u64 bufferSize);
GROUNDED_FUNCTION float textualReaderReadFloatWithOptionalIEEEHex(TextualReader* reader);
GROUNDED_FUNCTION s64 textualReaderReadSignedInteger(TextualReader* reader);
GROUNDED_FUNCTION u64 textualReaderReadUnsignedInteger(TextualReader* reader);
GROUNDED_FUNCTION u64 textualReaderReadHex(TextualReader* reader);


typedef struct SimpleWriter {
    BufferedStreamWriter w;
    u64 bytesWritten;
} SimpleWriter;

#define SIMPLE_WRITER_WRITE(writer, element) (simpleWriterWrite(writer, element, sizeof(*element)))
#define SIMPLE_WRITER_WRITE_ARRAY(writer, count, firstElement) (simpleWriterWrite(writer, firstElement, sizeof(*firstElement) * count))

GROUNDED_FUNCTION_INLINE SimpleWriter createSimpleWriter(BufferedStreamWriter w) {
    SimpleWriter result = {
        .w = w,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE void simpleWriterWrite(SimpleWriter* writer, const void* data, u64 size) {
    u8* src = (u8*)data;
    s64 sizeLeft = writer->w.end - writer->w.start;
    while(sizeLeft < (s64)size) {
        groundedCopyMemory(writer->w.start, src, sizeLeft);
        writer->bytesWritten += sizeLeft;
        writer->w.submit(&writer->w, writer->w.end);
        size -= sizeLeft;
        src = src + sizeLeft;
        sizeLeft = writer->w.end - writer->w.start;
    }
    groundedCopyMemory(writer->w.start, src, size);
    writer->w.start += size;
    writer->bytesWritten += size;
}

GROUNDED_FUNCTION_INLINE void simpleWriterWriteAligned(SimpleWriter* writer, const void* data, u64 size, u64 alignment) {
    ASSERT(IS_POW2(alignment));
    u8* src = (u8*)data;
    u8* alignedStartPos = (u8*)PTR_FROM_INT(ALIGN_UP_POW2(INT_FROM_PTR(writer->w.start), alignment));
    s64 sizeLeft = writer->w.end - alignedStartPos;
    while(sizeLeft < (s64)size) {
        groundedCopyMemory(alignedStartPos, src, sizeLeft);
        writer->bytesWritten += sizeLeft + (alignedStartPos - writer->w.start);
        writer->w.submit(&writer->w, writer->w.end);
        size -= sizeLeft;
        src = src + sizeLeft;
        alignedStartPos = writer->w.start;
        sizeLeft = writer->w.end - writer->w.start;
    }
    groundedCopyMemory(alignedStartPos, src, size);
    writer->w.start = alignedStartPos + size;
    writer->bytesWritten += size + (alignedStartPos - writer->w.start);
}

GROUNDED_FUNCTION_INLINE u64 simpleWriterGetOffset(SimpleWriter* writer) {
    return writer->bytesWritten;
}

GROUNDED_FUNCTION_INLINE void simpleWriterFlush(SimpleWriter* writer) {
    writer->w.submit(&writer->w, writer->w.start);
}

GROUNDED_FUNCTION_INLINE void simpleWriterClose(SimpleWriter* writer) {
    memoryStreamWriterClose(&writer->w);
}


typedef struct TextualWriter {
    SimpleWriter w;
} TextualWriter;

GROUNDED_FUNCTION_INLINE TextualWriter createTextualWriter(BufferedStreamWriter w) {
    TextualWriter result = {
        .w = createSimpleWriter(w),
    };
    return result;
}

GROUNDED_FUNCTION_INLINE void textualWriterWriteString(TextualWriter* writer, String8 s) {
    simpleWriterWrite(&writer->w, s.base, s.size);
}

GROUNDED_FUNCTION_INLINE void textualWriterWriteCstr(TextualWriter* writer, const char* s) {
    textualWriterWriteString(writer, str8FromCstr(s));
}

GROUNDED_FUNCTION void textualWriterWriteFloatWithIEEEHex(TextualWriter* writer, float f);
GROUNDED_FUNCTION void textualWriterWriteDoubleWithIEEEHex(TextualWriter* writer, double f);
GROUNDED_FUNCTION void textualWriterWriteSignedInteger(TextualWriter* writer, s64 i);
GROUNDED_FUNCTION void textualWriterWriteUnsignedInteger(TextualWriter* writer, u64 i);

GROUNDED_FUNCTION_INLINE void textualWriterFlush(TextualWriter* writer) {
    simpleWriterFlush(&writer->w);
}

GROUNDED_FUNCTION_INLINE void textualWriterClose(TextualWriter* writer) {
    simpleWriterClose(&writer->w);
}

#endif // GROUNDED_STREAM_H
