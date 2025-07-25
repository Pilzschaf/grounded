#include <grounded/string/grounded_string.h>
#include <grounded/memory/grounded_memory.h>

#include <stdarg.h>

#include "../../unicode_mappings.h"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOUNALIGNED
#include <grounded/string/stb_sprintf.h>
#undef STB_SPRINTF_IMPLEMENTATION
#undef STB_SPRINTF_NOUNALIGNED

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

GROUNDED_FUNCTION bool str8IsEqual(String8 a, String8 b) {
    if(a.size == b.size) {
        if(a.size == 0) {
            return true;
        }
        return groundedCompareMemory(a.base, b.base, a.size);
    }
    return false;
}

GROUNDED_FUNCTION int str8Compare(String8 a, String8 b) {
    u64 size = MIN(a.size, b.size);
    for(u64 i = 0; i < size; ++i) {
        if(a.base[i] < b.base[i]) {
            return -1;
        } else if(a.base[i] > b.base[i]) {
            return 1;
        }
    }
    return a.size < b.size ? -1 : ((a.size > b.size) ? 1 : 0);
}

GROUNDED_FUNCTION int str8CompareCaseInsensitive(String8 a, String8 b) {
    u64 offsetA = 0;
    u64 offsetB = 0;

    while(offsetA < a.size && offsetB < b.size) {
        StringDecode decodeA = strDecodeUtf8(a.base + offsetA, a.size - offsetA);
        offsetA += decodeA.size;
        StringDecode decodeB = strDecodeUtf8(b.base + offsetB, b.size - offsetB);
        offsetB += decodeB.size;
        u32 codepointA = strCodepointToLower(decodeA.codepoint);
        u32 codepointB = strCodepointToLower(decodeB.codepoint);
        if(codepointA < codepointB) {
            return -1;
        } else if(codepointA > codepointB) {
            return 1;
        }
    }
    return a.size < b.size ? -1 : ((a.size > b.size) ? 1 : 0);
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

static u32 lookupCodepointConversion(const UnicodeMapping* mappings, u32 mappingCount, u32 codepoint) {
    u32 result = UINT32_MAX;
    if(mappingCount > 0 && mappingCount <= INT32_MAX && codepoint >= mappings[0].source) {
        s32 left = 0;
        s32 right = MIN((codepoint - mappings[0].source), mappingCount - 1);
        while(left <= right) {
            s32 middle = left + (right - left) / 2;
            // Skewed midpoint: give more weight to the left side
            // int middle = left + (right - left) / 2 - (right - left) / 8;  // Adjust by a bias factor

            if(toLower[middle].source == codepoint) {
                result = toLower[middle].target;
                break;
            }
            if (toLower[middle].source < codepoint) {
                left = middle + 1;
            } else {
                right = middle - 1;
            }
        }
    }
    return result;
}

GROUNDED_FUNCTION bool str8IsLowercase(String8 str) {
    bool result = true;

    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;

        // Lookup in toLower table
        u32 target = lookupCodepointConversion(toLower, ARRAY_COUNT(toLower), decode.codepoint);
        if(target != UINT32_MAX) {
            result = false;
            break;
        }

        ptr += decode.size;
    }
    return result;
}

GROUNDED_FUNCTION bool str8IsUppercase(String8 str) {
    bool result = true;

    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;

        // Lookup in toUpper table
        u32 target = lookupCodepointConversion(toUpper, ARRAY_COUNT(toUpper), decode.codepoint);
        if(target != UINT32_MAX) {
            result = false;
            break;
        }

        ptr += decode.size;
    }
    return result;
}

GROUNDED_FUNCTION s64 str8DeltaToNextWordBoundary(String8 str, u64 cursor, s64 inc) {
    s64 result = 0;
    for(s64 byteOffset = (s64)cursor + inc; byteOffset >= 0 && (u64)byteOffset <= str.size; byteOffset += inc) {
        if(byteOffset == 0 || (u64)byteOffset == str.size) {
            result += byteOffset - (s64)cursor;
            break;
        }
        s64 off = inc > 0 ? -1 : 0;
        ASSERT(byteOffset+off < (s64)str.size);
        ASSERT(byteOffset+inc+off < (s64)str.size);
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
    u64 actualSize = stbsp_vsnprintf((char*)buffer, bufferSize, format, args);
    
    String8 result = {0};
    if (actualSize < bufferSize){
        // Release excess memory
        arenaPopTo(arena, buffer + actualSize + 1);
        result = str8FromBlock(buffer, actualSize);
    } else {
        // If first try failed, reset and try again with correct size
        arenaResetToMarker(firstBufferMarker);
        u8* fixedBuffer = ARENA_PUSH_ARRAY(arena, actualSize + 1, u8);
        u64 finalSize = stbsp_vsnprintf((char*)fixedBuffer, actualSize + 1, format, args2);
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

// Data for uppercase / lowercase conversion: https://www.unicode.org/Public/16.0.0/ucd/UnicodeData.txt
GROUNDED_FUNCTION String8 str8ToLower(struct MemoryArena* arena, String8 str) {
    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    u64 allocated = str.size*4+1;
    u8* memory = ARENA_PUSH_ARRAY(arena, allocated, u8);
    u8* dptr = memory;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;

        // Lookup in toLower table
        u32 target = lookupCodepointConversion(toLower, ARRAY_COUNT(toLower), decode.codepoint);
        if(target == UINT32_MAX) {
            // Character not found so we do not need to convert and simply copy source
            MEMORY_COPY(dptr, ptr, decode.size);
            dptr += decode.size;
        } else {
            u32 length = strEncodeUtf8(dptr, target);
            dptr += length;
        }

        ptr += decode.size;
    }

    // Null terminate and release overallocation
    ASSERT(dptr < memory + allocated);
    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, dptr+1);

    String8 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String8 str8ToUpper(struct MemoryArena* arena, String8 str) {
    u8* ptr = str.base;
    u8* opl = str.base + str.size;

    u64 allocated = str.size*4+1;
    u8* memory = ARENA_PUSH_ARRAY(arena, allocated, u8);
    u8* dptr = memory;

    for(;ptr < opl;) {
        StringDecode decode = strDecodeUtf8(ptr, (u32)(opl - ptr));
        if(decode.size == 0) break;

        // Lookup in toUpper table
        u32 target = lookupCodepointConversion(toUpper, ARRAY_COUNT(toUpper), decode.codepoint);
        if(target == UINT32_MAX) {
            // Character not found so we do not need to convert and simply copy source
            MEMORY_COPY(dptr, ptr, decode.size);
            dptr += decode.size;
        } else {
            u32 length = strEncodeUtf8(dptr, target);
            dptr += length;
        }

        ptr += decode.size;
    }

    // Null terminate and release overallocation
    ASSERT(dptr < memory + allocated);
    *dptr = 0;
    u64 stringCount = (u64)(dptr - memory);
    arenaPopTo(arena, dptr+1);

    String8 result = {memory, stringCount};
    return result;
}

GROUNDED_FUNCTION String8 str8ReplaceCharacter(struct MemoryArena* arena, String8 str, u8 old, u8 new) {
    String8 result = {0};
    result.base = ARENA_PUSH_ARRAY_NO_CLEAR(arena, str.size + 1, u8);
    result.size = str.size;
    for(u64 i = 0; i < str.size; ++i) {
        if(str.base[i] == old) {
            result.base[i] = new;
        } else {
            result.base[i] = str.base[i];
        }
    }
    result.base[result.size] = '\0';
    return result;
}

GROUNDED_FUNCTION String8 str8RemoveCharacter(struct MemoryArena* arena, String8 str, u8 character) {
    String8 result = {0};
    result.base = ARENA_PUSH_ARRAY_NO_CLEAR(arena, str.size + 1, u8);
    for(u64 i = 0; i < str.size; ++i) {
        if(str.base[i] != character) {
            result.base[result.size++] = str.base[i];
        }
    }
    result.base[result.size] = '\0';
    // Release potentially overallocated memory
    arenaPopTo(arena, result.base + result.size + 1);
    return result;
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

GROUNDED_FUNCTION String8List str8Split(MemoryArena* arena, String8 str, u8* splitCharacters, u64 splitCharacterCount) {
    String8List result = {0};

    u8* ptr = str.base;
    u8* wordFirst = ptr;
    u8* opl = str.base + str.size;
    for(;ptr < opl; ptr += 1) {
        u8 byte = *ptr;
        bool isSplitByte = false;
        for(u64 i = 0; i < splitCharacterCount; ++i) {
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

GROUNDED_FUNCTION String8* str8ListToArray(struct MemoryArena* arena, String8List* list) {
    String8* result = ARENA_PUSH_ARRAY(arena, list->numNodes, String8);
    String8Node* node = list->first;
    for(u64 i = 0; i < list->numNodes; ++i) {
        ASSUME(node) {
            result[i] = node->string;
            node = node->next;
        }
    }
    
    return result;
}

GROUNDED_FUNCTION String8* str8SplitToArray(MemoryArena* arena, String8 str, u8* splitCharacters, u64 splitCharacterCount, u64* outCount) {
    String8* result = 0;
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    String8List list = str8Split(scratch, str, splitCharacters, splitCharacterCount);
    if(list.numNodes) {
        result = ARENA_PUSH_ARRAY_NO_CLEAR(arena, list.numNodes, String8);
        if(!result) {
            list.numNodes = 0;
        } else {
            String8Node* node = list.first;
            for(u64 i = 0; i < list.numNodes; ++i) {
                result[i] = node->string;
                node = node->next;
            }
        }
    }

    if(outCount) {
        *outCount = list.numNodes;
    }
    arenaEndTemp(temp);
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

GROUNDED_FUNCTION StringDecode strDecodeUtf16(u16* string, s64 capacity) {
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

GROUNDED_FUNCTION u32 strCodepointToLower(u32 codepoint) {
    u32 result = lookupCodepointConversion(toLower, ARRAY_COUNT(toLower), codepoint);
    if(result == UINT32_MAX) {
        result = codepoint;
    }
    return result;
}

GROUNDED_FUNCTION u32 strCodepointToUpper(u32 codepoint) {
    u32 result = lookupCodepointConversion(toUpper, ARRAY_COUNT(toUpper), codepoint);
    if(result == UINT32_MAX) {
        result = codepoint;
    }
    return result;
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

#ifdef _WIN32
#include <windows.h>
GROUNDED_FUNCTION void groundedPrintString(String8 str) {
    DWORD bytesWritten = 0;
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdoutHandle != INVALID_HANDLE_VALUE) {
        WriteFile(stdoutHandle, str.base, (DWORD)str.size, &bytesWritten, 0);
    }
}
#else
#include <unistd.h>
GROUNDED_FUNCTION void groundedPrintString(String8 str) {
    write(1, str.base, str.size);
}
#endif

GROUNDED_FUNCTION void groundedPrintStringf(const char* str, ...) {
    va_list args;
    va_start(args, str);
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    String8 result = str8FromFormatVaList(scratch, str, args);
    groundedPrintString(result);
    
    arenaEndTemp(temp);
    va_end(args);
}
