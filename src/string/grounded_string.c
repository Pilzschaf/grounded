#include <grounded/string/grounded_string.h>
#include <grounded/memory/grounded_memory.h>

#include <stdarg.h>
#include <stdio.h>

GROUNDED_FUNCTION String8 str8Prefix(String8 str, u64 size) {
    ASSERT(size <= str.size);
    u64 sizeClamped = CLAMP_TOP(size, str.size);
    String8 result = {str.base, sizeClamped};
    return result;
}

GROUNDED_FUNCTION String8 str8Chop(String8 str, u64 amount) {
    ASSERT(amount <= str.size);
    u64 amountClamped = CLAMP_TOP(amount, str.size);
    u64 remainingSize = str.size - amountClamped;
    String8 result = {str.base, remainingSize};
    return result;
}

GROUNDED_FUNCTION String8 str8PostFix(String8 str, u64 size) {
    ASSERT(size <= str.size);
    u64 sizeClamped = CLAMP_TOP(size, str.size);
    u64 skipTo = str.size - sizeClamped;
    String8 result = {str.base + skipTo, sizeClamped};
    return result;
}

GROUNDED_FUNCTION String8 str8Skip(String8 str, u64 amount) {
    ASSERT(amount <= str.size);
    u64 amountClamped = CLAMP_TOP(amount, str.size);
    u64 remainingSize = str.size - amountClamped;
    String8 result = {str.base + amountClamped, remainingSize};
    return result;
}

GROUNDED_FUNCTION String8 str8Substring(String8 str, u64 first, u64 last) {
    ASSERT(first < last);
    ASSERT(last - first <= str.size - first);
    u64 sizeClamped = CLAMP_TOP(last - first, str.size - first);
    String8 result = {str.base + first, sizeClamped};
    return result;
}

GROUNDED_FUNCTION bool str8Compare(String8 a, String8 b) {
    if(a.size == b.size) {
        return groundedCompareMemory(a.base, b.base, a.size);
    }
    return false;
}

GROUNDED_FUNCTION u64 str8GetFirstOccurence(String8 str, char c) {
    for(u64 i = 0; i < str.size; ++i) {
        if(str.base[i] == c) return i;
    }
    return UINT64_MAX;
}

GROUNDED_FUNCTION u64 str8GetLastOccurence(String8 str, char c) {
    for(u64 i = str.size; i > 0; --i) {
        if(str.base[i-1] == c) return i-1;
    }
    return UINT64_MAX;
}

GROUNDED_FUNCTION bool str8IsPrefixOf(String8 prefix, String8 str) {
    bool result = false;
    if(prefix.size <= str.size) {
        result = true;
        for(u64 i = 0; i < prefix.size; ++i) {
            if(prefix.base[i] != str.base[i]) {
                result = false;
                break;
            }
        }
    }
    return result;
}

GROUNDED_FUNCTION bool str8IsPostfixOf(String8 postfix, String8 str) {
    bool result = false;
    if(postfix.size <= str.size) {
        result = true;
        for(u64 i = 0; i < postfix.size; ++i) {
            if(postfix.base[postfix.size-i-1] != str.base[str.size-i-1]) {
                result = false;
                break;
            }
        }
    }
    return result;
}

GROUNDED_FUNCTION bool str8IsSubstringOf(String8 substring, String8 str) {
    bool result = false;
    if(substring.size <= str.size) {
        for(u64 i = 0; i < (str.size - substring.size + 1); ++i) {
            result = true;
            for(u64 j = 0; j < substring.size; ++j) {
                ASSERT(i+j < str.size);
                if(substring.base[j] != str.base[i+j]) {
                    result = false;
                    break;
                }
            }
            if(result) {
                break;
            }
        }
    }
    return result;
}

GROUNDED_FUNCTION s64 str8DeltaToNextWordBoundary(String8 str, u64 cursor, s64 inc) {
    s64 result = 0;
    for(s64 byteOffset = (s64)cursor + inc; byteOffset >= 0 && byteOffset <= str.size; byteOffset += inc) {
        if(byteOffset == 0 || byteOffset == str.size) {
            result += byteOffset - (s64)cursor;
            break;
        }
        s64 off = inc > 0 ? -1 : 0;
        ASSERT(byteOffset+off < str.size);
        ASSERT(byteOffset+inc+off < str.size);
        ASSERT(byteOffset+off >= 0);
        ASSERT(byteOffset+inc+off >= 0);
        if(!asciiCharIsBoundary(str.base[byteOffset+off]) && asciiCharIsBoundary(str.base[byteOffset+inc+off])) {
            result += byteOffset - (s64)cursor;
            break;
        }
    }
    return result;
}

String8 str8FromFormatVaList(struct MemoryArena* arena, const char* format, va_list args) {
    // In case we need to try a second time
    va_list args2;
    va_copy(args2, args);
    
    // Try to build the string in 1024 bytes
    u64 bufferSize = 1024;
    ArenaMarker firstBufferMarker = arenaCreateMarker(arena);
    u8* buffer = ARENA_PUSH_ARRAY(arena, bufferSize, u8);
    u64 actualSize = vsnprintf((char*)buffer, bufferSize, format, args);
    
    String8 result = {0};
    if (actualSize < bufferSize){
        // Release excess memory
        arenaPopTo(arena, buffer + actualSize + 1);
        result = str8FromBlock(buffer, actualSize);
    } else {
        // If first try failed, reset and try again with correct size
        arenaResetToMarker(firstBufferMarker);
        u8* fixedBuffer = ARENA_PUSH_ARRAY(arena, actualSize + 1, u8);
        u64 finalSize = vsnprintf((char*)fixedBuffer, actualSize + 1, format, args2);
        result = str8FromBlock(fixedBuffer, finalSize);
    }
    
    va_end(args2);
    
    return result;
}

GROUNDED_FUNCTION String8 str8FromFormat(struct MemoryArena* arena, const char* format, ...) {
    va_list args;
    va_start(args, format);
    String8 result = str8FromFormatVaList(arena, format, args);
    va_end(args);
    return result;
}

GROUNDED_FUNCTION String8 str8Copy(MemoryArena* arena, String8 str) {
    String8 result;
    result.base = ARENA_PUSH_ARRAY(arena, str.size, u8);
    result.size = str.size;
    // Copy of 0 bytes is UB
    if(str.size > 0) {
        MEMORY_COPY(result.base, str.base, str.size);
    }
    return result;
}

GROUNDED_FUNCTION String8 str8CopyAndNullTerminate(MemoryArena* arena, String8 str) {
    String8 result;
    result.base = ARENA_PUSH_ARRAY(arena, str.size+1, u8);
    result.size = str.size;
    MEMORY_COPY(result.base, str.base, str.size);
    result.base[result.size] = 0;
    return result;
}

GROUNDED_FUNCTION char* str8GetCstr(MemoryArena* arena, String8 str) {
    char* result = ARENA_PUSH_ARRAY(arena, str.size+1, char);
    MEMORY_COPY(result, str.base, str.size);
    result[str.size] = '\0';
    return result;
}

GROUNDED_FUNCTION char* str8GetCstrOrNull(struct MemoryArena* arena, String8 str) {
    if(str.size == 0) {
        return 0;
    } else {
        return str8GetCstr(arena, str);
    }
}

//////////////
// String list
GROUNDED_FUNCTION void str8ListPushExplicit(String8List* list, String8 str, String8Node* nodeMemory) {
    nodeMemory->string = str;
    if(!list->first) {
        list->first = list->last = nodeMemory;
    } else {
        list->last->next = nodeMemory;
        list->last = nodeMemory;
        nodeMemory->next = 0;
    }
    list->numNodes += 1;
    list->totalSize += str.size;
}

GROUNDED_FUNCTION void str8ListPush(MemoryArena* arena, String8List* list, String8 str) {
    String8Node* node = ARENA_PUSH_ARRAY(arena, 1, String8Node);
    str8ListPushExplicit(list, str, node);
}

GROUNDED_FUNCTION void str8ListPushCopy(struct MemoryArena* arena, String8List* list, String8 str) {
    String8Node* node = ARENA_PUSH_ARRAY(arena, 1, String8Node);
    str8ListPushExplicit(list, str8Copy(arena, str), node);
}

GROUNDED_FUNCTION void str8ListPushCopyAndNullTerminate(struct MemoryArena* arena, String8List* list, String8 str) {
    String8Node* node = ARENA_PUSH_ARRAY(arena, 1, String8Node);
    str8ListPushExplicit(list, str8CopyAndNullTerminate(arena, str), node);
}

GROUNDED_FUNCTION String8 str8ListJoin(MemoryArena* arena, String8List* list, StringJoin* optionalJoin) {
    static StringJoin dummyJoin = {0};
    StringJoin* join = optionalJoin;
    if(join == 0) {
        join = &dummyJoin;
    }

    u64 size = (join->pre.size + join->post.size + join->mid.size*(list->numNodes - 1) + list->totalSize);

    u8* str = ARENA_PUSH_ARRAY(arena, size + 1, u8);
    u8* ptr = str;

    // write pre
    if(join->pre.size) {
        MEMORY_COPY(ptr, join->pre.base, join->pre.size);
        ptr += join->pre.size;
    }

    // write mid
    bool isMid = false;
    for(String8Node* node = list->first; node != 0; node = node->next) {
        if(isMid && join->mid.size) {
            MEMORY_COPY(ptr, join->mid.base, join->mid.size);
            ptr += join->mid.size;
        }
        if(node->string.size) {
            MEMORY_COPY(ptr, node->string.base, node->string.size);
            ptr += node->string.size;
        }

        isMid = node->next != 0;
    }

    // write post
    if(join->post.size) {
        MEMORY_COPY(ptr, join->post.base, join->post.size);
        ptr += join->post.size;
    }

    String8 result = {str, (ptr - str)};
    str[ptr - str] = '\0';
    return result;
}

GROUNDED_FUNCTION String8List str8Split(MemoryArena* arena, String8 str, u8* splitCharacters, u64 count) {
    String8List result = {0};

    u8* ptr = str.base;
    u8* wordFirst = ptr;
    u8* opl = str.base + str.size;
    for(;ptr < opl; ptr += 1) {
        u8 byte = *ptr;
        bool isSplitByte = false;
        for(u64 i = 0; i < count; ++i) {
            if(byte == splitCharacters[i]) {
                isSplitByte = true;
                break;
            }
        }
        if(isSplitByte) {
            if(wordFirst < ptr) {
                str8ListPush(arena, &result, str8FromRange(wordFirst, ptr));
            }
            wordFirst = ptr+1;
        }
    }

    // Emit final word
    if(wordFirst < ptr) {
        str8ListPush(arena, &result, str8FromRange(wordFirst, ptr));
    }

    return result;
}

//////////
// Unicode

// This implementation is quite efficient for strings containing mostly ASCII
// but suffers from branch mispredictions in unicode heavy strings
// Alternative would be SIMD version. See simdjson https://www.youtube.com/watch?v=wlvKAT7SZIQ
GROUNDED_FUNCTION bool str8IsValidUtf8(String8 str) {
    u64 sizeLeft = str.size;
    u8* currentyBase = str.base;
    
    while(sizeLeft > 0) {
        if(currentyBase[0] < 0x80) {
            currentyBase++;
            sizeLeft--;
            continue;
        } else if((currentyBase[0] & 0b11100000) == 0b11000000) {
            if(sizeLeft < 2) return false;
            if((currentyBase[1] & 0b11000000) != 0b10000000) return false;
            sizeLeft -= 2;
            currentyBase += 2;
        } else if((currentyBase[0] & 0b11110000) == 0b11100000) {
            if(sizeLeft < 3) return false;
            if((currentyBase[1] & 0b11000000) != 0b10000000) return false;
            if((currentyBase[2] & 0b11000000) != 0b10000000) return false;
            sizeLeft -= 3;
            currentyBase += 3;
        } else if((currentyBase[0] & 0b11111000) == 0b11110000) {
            if(sizeLeft < 4) return false;
            if((currentyBase[1] & 0b11000000) != 0b10000000) return false;
            if((currentyBase[2] & 0b11000000) != 0b10000000) return false;
            if((currentyBase[3] & 0b11000000) != 0b10000000) return false;
            sizeLeft -= 4;
            currentyBase += 4;
        } else {
            // Invalid first byte
            return false;
        }
    }
    return true;
}

GROUNDED_FUNCTION StringDecode strDecodeUtf8(u8* string, u32 capacity) {
    static u8 lengthTable[] = {
        1, 1, 1, 1, // 000xx
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        0, 0, 0, 0, // 100xx
        0, 0, 0, 0,
        2, 2, 2, 2, // 110xx
        3, 3,       // 1110x
        4,          // 11110
        0,          // 11111
    };
    static u8 firstByteMask[] = {0, 0x7F, 0x1F, 0x0F, 0x07 };
    static u8 finalShift[] = {0, 18, 12, 6, 0};
    StringDecode result = {0};
    if(capacity > 0) {
        result.codepoint = '#';
        result.size = 1;

        u8 byte = string[0];
        u8 length = lengthTable[byte >> 3];
        if(length > 0 && length <= capacity) {
            u32 codepoint = (byte & firstByteMask[length]) << 18;
            switch(length-1) {
                case 3: codepoint |= ((string[3] & 0x3F) << 0); // fallthrough
                case 2: codepoint |= ((string[2] & 0x3F) << 6); // fallthrough
                case 1: codepoint |= ((string[1] & 0x3F) << 12); // fallthrough
                case 0: codepoint >>= finalShift[length];
            }
            result.codepoint = codepoint;
            result.size = length;
        }
    }

    return result;
}

GROUNDED_FUNCTION u32 strEncodeUtf8(u8* dst, u32 codepoint) {
    u32 size = 0;
    if(codepoint < (1 << 7)) {
        dst[0] = codepoint;
        size = 1;
    } else if(codepoint < (1 << 11)) {
        dst[0] = 0xC0 | (codepoint >> 6);
        dst[1] = 0x80 | (codepoint & 0x3F);
        size = 2;
    } else if(codepoint < (1 << 16)) {
        dst[0] = 0xE0 | (codepoint >> 12);
        dst[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[2] = 0x80 | (codepoint & 0x3F);
        size = 3;
    } else if(codepoint < (1 << 21)) {
        dst[0] = 0xF0 | (codepoint >> 18);
        dst[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        dst[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        dst[3] = 0x80 | (codepoint & 0x3F);
        size = 4;
    } else {
        dst[0] = '#';
        size = 1;
    }
    return size;
}

GROUNDED_FUNCTION StringDecode strDecodeUtf16(u16* string, u32 capacity) {
    StringDecode result = {0};
    if(capacity > 0) {
        u16 x = string[0];
        if(x < 0xD800 || 0xDFFF < x) {
            result.codepoint = x;
            result.size = 1;
        } else if(capacity >= 2) {
            u16 y = string[1];
            if(0xD800 <= x && x < 0xDC00 && 0xDC00 <= y && y < 0xE000) {
                u16 xj = x - 0xD800;
                u16 yj = y - 0xDC00;
                u32 xy = (xj << 10) | yj;
                result.codepoint = xy + 0x10000;
                result.size = 2;
            }
        }
    }
    return result;
}

GROUNDED_FUNCTION u32 strEncodeUtf16(u16* dst, u32 codepoint) {
    u32 size = 0;
    if(codepoint < 0x10000) {
        dst[0] = codepoint;
        size = 1;
    } else {
        u32 cpj = codepoint - 0x10000;
        dst[0] = (cpj >> 10) + 0xD800;
        dst[1] = (cpj & 0x3FF) + 0xDC00;
        size = 2;
    }
    return size;
}

GROUNDED_FUNCTION String8 str8FromStr16(MemoryArena* arena, String16 str) {
    u64 allocated = str.size*3+1;
    u8* memory = ARENA_PUSH_ARRAY(arena, allocated, u8);
    u8* dptr = memory;
    u16* ptr = str.base;
    u16* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf16(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;
        u32 encodedSize = strEncodeUtf8(dptr, decode.codepoint);
        ptr += decode.size;
        dptr += encodedSize;
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, dptr+1);

    String8 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String8 str8FromStr32(MemoryArena* arena, String32 str) {
    u64 allocated = str.size*4+1;
    u8* memory = ARENA_PUSH_ARRAY(arena, allocated, u8);
    u8* dptr = memory;
    u32* ptr = str.base;
    u32* opl = str.base + str.size;

    for(;ptr < opl;) {
        u32 size = strEncodeUtf8(dptr, *ptr);
        ptr += 1;
        dptr += size;
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, dptr+1);

    String8 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String16 str16FromStr8(MemoryArena* arena, String8 str) {
    u64 allocated = str.size*2+1;
    u16* memory = ARENA_PUSH_ARRAY(arena, allocated, u16);
    u16* dptr = memory;
    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;
        u32 encodedSize = strEncodeUtf16(dptr, decode.codepoint);
        ptr += decode.size;
        dptr += encodedSize;
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, (u8*)(dptr+1));

    String16 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String16 str16FromStr32(struct MemoryArena* arena, String32 str) {
    u64 allocated = str.size*2+1;
    u16* memory = ARENA_PUSH_ARRAY(arena, allocated, u16);
    u16* dptr = memory;
    u32* ptr = str.base;
    u32* opl = str.base + str.size;

    for(;ptr < opl;) {
        u32 encodedSize = strEncodeUtf16(dptr, *ptr);
        dptr += encodedSize;
        ptr += 1;
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, (u8*)(dptr+1));
    
    String16 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String32 str32FromStr8(MemoryArena* arena, String8 str) {
    u64 allocated = str.size+1;
    u32* memory = ARENA_PUSH_ARRAY(arena, allocated, u32);
    u32* dptr = memory;
    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;
        *dptr = decode.codepoint;
        ptr += decode.size;
        dptr += 1;
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, (u8*)(dptr+1));

    String32 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String32 str32FromStr16(struct MemoryArena* arena, String16 str) {
    u64 allocated = str.size+1;
    u32* memory = ARENA_PUSH_ARRAY(arena, allocated, u32);
    u32* dptr = memory;
    u16* ptr = str.base;
    u16* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf16(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;
        *dptr = decode.codepoint;
        ptr += decode.size;
        dptr += 1;    
    }

    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, (u8*)(dptr+1));

    String32 result = {memory, stringCount};
    return result;
}