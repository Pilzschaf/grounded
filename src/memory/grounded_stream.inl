#include <grounded/memory/grounded_stream.h>

#include <stdio.h>
#include <stdlib.h>

// Returns 0 and does not advance if reader is at whitespace
GROUNDED_FUNCTION u64 textualReaderReadUntilWhitespace(TextualReader* reader, u8* buffer, u64 bufferSize) {
    for(u64 i = 0; i < bufferSize; ++i) {
        if(reader->r.cursor >= reader->r.end) {
            reader->r.refill(&reader->r);
        }
        if(isspace(reader->r.cursor[0])) {
            return i;
        }
        // This is not a good idea as there is no way to get the EOF error on a successive read
        /*if(reader->r.cursor[0] == '\0') {
            // We did hit \0 and read more than one byte in this case we handle this as whitespace even if this is EOF and ignore the EOF error for now
            if(i > 0 && reader->r.error == PAST_EOF) {
                reader->r.error = NO_ERROR;
            }
            return i;
        }*/
        buffer[i] = reader->r.cursor[0];
        reader->r.cursor++;
    }
    return bufferSize;
}

GROUNDED_FUNCTION float textualReaderReadFloatWithOptionalIEEEHex(TextualReader* reader) {
    u8 buffer[64]; // This should be plenty to read a single number.
    float result = 0.0f;
    while(true) {
        u64 size = textualReaderReadUntilWhitespace(reader, buffer, ARRAY_COUNT(buffer)-1);
        buffer[size] = '\0';
        if(size == 0 && reader->r.error == GROUNDED_STREAM_SUCCESS) {
            // We are at whitespace so skip this character
            reader->r.cursor++;
        } else {
            result = strtof((const char*)buffer, 0);
            break;
        }
    }

    // Here we must seek until \n and check whether a hex representation is following
    while(true) {
        u64 size = textualReaderReadUntilWhitespace(reader, buffer, ARRAY_COUNT(buffer)-1);
        buffer[size] = '\0';
        if(size == 0) {
            char c = reader->r.cursor[0];
            reader->r.cursor++;
            if(c == '\n') {
                // Next line is beginning so return current result
                return result;
            }
        } else if(buffer[0] == ':') {
            // This is followed by an hexadecimal representation
            //TODO: This does not handle the error correctly where there is no number after :
            
            u32 hex = (u32)textualReaderReadHex(reader);
            union TypeConvert {
                u32 h;
                float f;
            };
            union TypeConvert convert;
            convert.h = hex;
            result = convert.f;
            return result;
        } else {
            // TODO: This is actually a parsing error. Maybe set reader into an error state
            return result;
        }
    }
}

GROUNDED_FUNCTION u64 textualReaderReadUnsignedInteger(TextualReader* reader) {
    u8 buffer[64]; // This should be plenty to read a single number.
    while(true) {
        u64 size = textualReaderReadUntilWhitespace(reader, buffer, ARRAY_COUNT(buffer)-1);
        buffer[size] = '\0';
        if(size == 0 && reader->r.error == GROUNDED_STREAM_SUCCESS) {
            // We are at whitespace so skip this character
            reader->r.cursor++;
        } else {
            u64 result = strtoul((const char*)buffer, 0, 10);
            return result;
        }
    }
}

GROUNDED_FUNCTION u64 textualReaderReadHex(TextualReader* reader) {
    u8 buffer[64]; // This should be plenty to read a single number.
    while(true) {
        u64 size = textualReaderReadUntilWhitespace(reader, buffer, ARRAY_COUNT(buffer)-1);
        buffer[size] = '\0';
        if(size == 0 && reader->r.error == GROUNDED_STREAM_SUCCESS) {
            // We are at whitespace so skip this character
            reader->r.cursor++;
        } else {
            u64 result = strtoul((const char*)buffer, 0, 16);
            return result;
        }
    }
}

GROUNDED_FUNCTION void textualWriterWriteFloatWithIEEEHex(TextualWriter* writer, float f) {
    u8 buffer[256];
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%f", f);
    textualWriterWriteCstr(writer, (const char*)buffer);
    textualWriterWriteCstr(writer, " : ");
    union TypeConvert {
        u32 h;
        float f;
    };
    union TypeConvert convert;
    convert.f = f;
    u32 h = convert.h;
    //snprintf((char*)buffer, ARRAY_COUNT(buffer), "%08x", h);
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%x", h);
    textualWriterWriteCstr(writer, (const char*)buffer);
}

GROUNDED_FUNCTION void textualWriterWriteDoubleWithIEEEHex(TextualWriter* writer, double f) {
    u8 buffer[256];
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%f", f);
    textualWriterWriteCstr(writer, (const char*)buffer);
    textualWriterWriteCstr(writer, " : ");
    union TypeConvert {
        u64 h;
        double f;
    };
    union TypeConvert convert;
    convert.f = f;
    u64 h = convert.h;
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%lx", h);
    textualWriterWriteCstr(writer, (const char*)buffer);
}

GROUNDED_FUNCTION void textualWriterWriteSignedInteger(TextualWriter* writer, s64 i) {
    u8 buffer[256];
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%li", i);
    textualWriterWriteCstr(writer, (const char*)buffer);
}

GROUNDED_FUNCTION void textualWriterWriteUnsignedInteger(TextualWriter* writer, u64 i) {
    u8 buffer[256];
    snprintf((char*)buffer, ARRAY_COUNT(buffer), "%lu", i);
    textualWriterWriteCstr(writer, (const char*)buffer);
}

