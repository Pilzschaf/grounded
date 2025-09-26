#ifndef GROUNDED_STRING_H
#define GROUNDED_STRING_H

#include "../grounded.h"

#include <string.h> // Required for strlen
#include <math.h> // Required for powf
#include <wchar.h> // Required for wcslen

GROUNDED_FUNCTION_INLINE bool asciiCharIsBoundary(u8 character) {
    bool isBoundary = character == ' ' || character == '\t' || character == '/' || character == '\n';
    return isBoundary;
}

GROUNDED_FUNCTION_INLINE u64 lengthOfCString(const char* cstr) {
    #ifdef GROUNDED_NO_STDLIB
    u64 result = 0;
    while(cstr[result]) {
        result++;
    }
    return  result;
    #else
    return strlen(cstr);
    #endif
}

GROUNDED_FUNCTION_INLINE String8 str8FromBlock(u8* str, u64 size) {
    String8 result = {str, size};
    return result;
}

GROUNDED_FUNCTION_INLINE String8 str8FromRange(u8* first, u8* opl) {
    ASSERT(first <= opl);
    String8 result = {first, (u64)(opl - first)};
    return result;
}

GROUNDED_FUNCTION_INLINE String8 str8FromCstr(const char* cstr) {
    if(!cstr) return EMPTY_STRING8;
    return str8FromBlock((u8*)cstr, lengthOfCString(cstr));
}

GROUNDED_FUNCTION_INLINE bool str8IsEmpty(String8 str) {
    return str.base == 0 || str.size == 0;
}

GROUNDED_FUNCTION String8 str8Prefix(String8 str, u64 size);
GROUNDED_FUNCTION String8 str8Chop(String8 str, u64 amount);
GROUNDED_FUNCTION String8 str8PostFix(String8 str, u64 size);
GROUNDED_FUNCTION String8 str8Skip(String8 str, u64 amount);
GROUNDED_FUNCTION String8 str8Substring(String8 str, u64 first, u64 last);
GROUNDED_FUNCTION String8 str8Join(struct MemoryArena* arena, String8 first, String8 second);
GROUNDED_FUNCTION bool str8IsEqual(String8 a, String8 b);
GROUNDED_FUNCTION int str8Compare(String8 a, String8 b);
GROUNDED_FUNCTION int str8CompareCaseInsensitive(String8 a, String8 b);
GROUNDED_FUNCTION u64 str8GetFirstOccurence(String8 str, char c); // Returns UINT64_MAX if not found
GROUNDED_FUNCTION u64 str8GetLastOccurence(String8 str, char c); // Returns UINT64_MAX if not found
GROUNDED_FUNCTION bool str8IsPrefixOf(String8 prefix, String8 str);
GROUNDED_FUNCTION bool str8IsPostfixOf(String8 postfix, String8 str);
GROUNDED_FUNCTION bool str8IsSubstringOf(String8 substring, String8 str);
GROUNDED_FUNCTION bool str8IsLowercase(String8 str);
GROUNDED_FUNCTION bool str8IsUppercase(String8 str);
GROUNDED_FUNCTION s64 str8DeltaToNextWordBoundary(String8 str, u64 cursor, s64 inc); // Returns a modified delta extended to the next word boundary of str

struct MemoryArena;
GROUNDED_FUNCTION String8 str8FromFormat(struct MemoryArena* arena, const char* format, ...);
GROUNDED_FUNCTION String8 str8Copy(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION String8 str8CopyAndNullTerminate(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION char* str8GetCstr(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION char* str8GetCstrOrNull(struct MemoryArena* arena, String8 str);

// Does not take into account locale specific conversion rules
GROUNDED_FUNCTION String8 str8ToLower(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION String8 str8ToUpper(struct MemoryArena* arena, String8 str);

GROUNDED_FUNCTION String8 str8ReplaceCharacter(struct MemoryArena* arena, String8 str, u8 oldCharacter, u8 newCharacter);
// Removes all occurences of character from str and copies it into new 0-terminated output string
GROUNDED_FUNCTION String8 str8RemoveCharacter(struct MemoryArena* arena, String8 str, u8 character);

GROUNDED_FUNCTION void str8ListPushExplicit(String8List* list, String8 str, String8Node* nodeMemory);
GROUNDED_FUNCTION void str8ListPush(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION void str8ListPushCopy(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION void str8ListPushCopyAndNullTerminate(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION String8 str8ListJoin(struct MemoryArena* arena, String8List* list, StringJoin* optionalJoin);
GROUNDED_FUNCTION String8List str8Split(struct MemoryArena* arena, String8 str, u8* splitCharacters, u64 splitCharacterCount);
GROUNDED_FUNCTION String8* str8ListToArray(struct MemoryArena* arena, String8List* list);
GROUNDED_FUNCTION String8* str8SplitToArray(struct MemoryArena* arena, String8 str, u8* splitCharacters, u64 splitCharacterCount, u64* outCount);

// Size in number of u16
GROUNDED_FUNCTION_INLINE String16 str16FromBlock(u16* str, u64 size) {
    String16 result = {str, size};
    return result;
}

GROUNDED_FUNCTION_INLINE String16 str16FromRange(u16* first, u16* opl) {
    ASSERT(first <= opl);
    String16 result = {first, (u64)(opl - first)};
    return result;
}

GROUNDED_FUNCTION_INLINE String16 str16FromWcstr(const wchar_t* wcstr) {
    if(!wcstr) return EMPTY_STRING16;
    return str16FromBlock((u16*)wcstr, wcslen(wcstr));
}

// Size in number of u32
GROUNDED_FUNCTION_INLINE String32 str32FromBlock(const u32* str, u64 size) {
    String32 result = {(u32*)str, size};
    return result;
}

GROUNDED_FUNCTION_INLINE String32 str32FromRange(u32* first, u32* opl) {
    ASSERT(first <= opl);
    String32 result = {first, (u64)(opl - first)};
    return result;
}

//////////
// Unicode
GROUNDED_FUNCTION bool str8IsValidUtf8(String8 str);

// Single codepoint encoding/decoding
// In error case result.size is 0
GROUNDED_FUNCTION_INLINE StringDecode strDecodeUtf8(u8* string, s64 capacity); // result.size is the number of bytes that have been advanced
// dst must have at least 4 bytes of space available
GROUNDED_FUNCTION u32 strEncodeUtf8(u8* dst, u32 codepoint); // Returns bytes advanced in dst. Maximum possible advance is 4 bytes.
// Capacity is the number of u16 left. In error case result.size is 0
GROUNDED_FUNCTION StringDecode strDecodeUtf16(u16* string, s64 capacity); // result.size is the number of u16 that have been advanced
// dst must have at least 4 bytes (2 u16) of space available
GROUNDED_FUNCTION u32 strEncodeUtf16(u16* dst, u32 codepoint); // Returns u16 advanced in dst. Maximum possible advance is 2

// Returns UINT32_MAX if codepoint can not be converted (not applicable or already correct case)
GROUNDED_FUNCTION u32 strCodepointToLower(u32 codepoint);
GROUNDED_FUNCTION u32 strCodepointToUpper(u32 codepoint);

// All functions 0-terminate the result string
GROUNDED_FUNCTION String8 str8FromStr16(struct MemoryArena* arena, String16 str);
GROUNDED_FUNCTION String8 str8FromStr32(struct MemoryArena* arena, String32 str);
GROUNDED_FUNCTION String16 str16FromStr8(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION String16 str16FromStr32(struct MemoryArena* arena, String32 str);
GROUNDED_FUNCTION String32 str32FromStr8(struct MemoryArena* arena, String8 str);
GROUNDED_FUNCTION String32 str32FromStr16(struct MemoryArena* arena, String16 str);

GROUNDED_FUNCTION_INLINE StringDecode strDecodeUtf8(u8* string, s64 capacity) {
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

////////
// Atoms
#define CREATE_ATOM_FROM_LITERAL(l) createAtom(STR8_LITERAL(l))

GROUNDED_FUNCTION_INLINE u64 atomHashBytes(const void* p, u64 length) {
    u64 hash = 0xcBF29CE484222325;
    const u8* buffer = (const u8*)p;
    for(u64 i = 0; i < length; i++) {
        hash ^= buffer[i];
        hash *= 0x100000001B3;
        hash ^= hash >> 32;
    }
    return hash;
}

GROUNDED_FUNCTION_INLINE StringAtom createAtom(String8 string) {
    StringAtom atom;
    atom.string = string;
    atom.hash = atomHashBytes(atom.string.base, atom.string.size);
    return atom;
}

GROUNDED_FUNCTION_INLINE bool compareAtoms(StringAtom a0, StringAtom a1) {
    if(a0.hash == a1.hash) {
        // Hash the same so check the string to make sure
        return str8IsEqual(a0.string, a1.string);
    }
    return false;
}

//TODO: String conversion functions like strToU64 etc.
/*GROUNDED_FUNCTION_INLINE u64 str8ToU64(String8 str) {
    u64 result = 0;
    for (u64 i = 0; i < str.size; i++) {
        u8 c = str.base[i];
        if(c <= '9' && c >= '0') {
            result = result * 10 + (c - '0');
        }
    }
    return result;
}*/

GROUNDED_FUNCTION_INLINE bool isSpace(int c) {
    return (c == ' '  || c == '\t' || c == '\n' || 
            c == '\v' || c == '\f' || c == '\r');
}

GROUNDED_FUNCTION_INLINE bool isDigit(int c) {
    return (c >= '0' && c <= '9');
}

GROUNDED_FUNCTION_INLINE float pow10f(int exp) {
    float base = 10.0f;
    float result = 1.0f;

    if (exp < 0) {
        exp = -exp;
        base = 0.1f;
    }

    while (exp) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }

    return result;
}

GROUNDED_FUNCTION_INLINE float str8ToFloat(String8 str) {
    u8* p = &str.base[0];
    int sign = 1;
    float result = 0.0f;
    float fraction = 0.1f;
    int exponent_sign = 1;
    int exponent_value = 0;

    // Skip leading whitespaces
    while (isSpace((unsigned char)*p)) {
        p++;
    }

    // Handle sign
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    // Parse integer part
    while (isDigit((unsigned char)*p)) {
        result = result * 10.0f + (*p - '0');
        p++;
    }

    // Parse fractional part
    if (*p == '.') {
        p++;
        while (isDigit((unsigned char)*p)) {
            result += (*p - '0') * fraction;
            fraction *= 0.1f;
            p++;
        }
    }

    // Parse exponent part (scientific notation)
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-') {
            exponent_sign = -1;
            p++;
        } else if (*p == '+') {
            p++;
        }
        while (isDigit((unsigned char)*p)) {
            exponent_value = exponent_value * 10 + (*p - '0');
            p++;
        }
    }

    // Apply exponent
    if (exponent_value) {
        result *= pow10f(exponent_sign * exponent_value);
    }

    return sign * result;
}

GROUNDED_FUNCTION_INLINE u64 str8ToU64(String8 str, u32 base) {
    u8* p = str.base;
    u64 result = 0;

    if (base < 2 || base > 36) {
        return 0; // invalid base
    }

    // Skip leading whitespace
    while (isSpace((unsigned char)*p)) {
        p++;
    }

    while (*p) {
        u8 c = *p;
        u32 digit = 255;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'z') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'Z') {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        p++;
    }

    return result;
}

GROUNDED_FUNCTION void groundedPrintString(String8 str);
GROUNDED_FUNCTION void groundedPrintStringf(const char* str, ...);

#endif // GROUNDED_STRING_H
