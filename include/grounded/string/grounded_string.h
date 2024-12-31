#ifndef GROUNDED_STRING_H
#define GROUNDED_STRING_H

#include "../grounded.h"

#include <string.h> // Required for strlen

GROUNDED_FUNCTION_INLINE bool asciiCharIsBoundary(u8 character) {
    bool isBoundary = character == ' ' || character == '\t' || character == '/' || character == '\n';
    return isBoundary;
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
    return str8FromBlock((u8*)cstr, strlen(cstr));
}

GROUNDED_FUNCTION_INLINE bool str8IsEmpty(String8 str) {
    return str.base == 0 || str.size == 0;
}

GROUNDED_FUNCTION String8 str8Prefix(String8 str, u64 size);
GROUNDED_FUNCTION String8 str8Chop(String8 str, u64 amount);
GROUNDED_FUNCTION String8 str8PostFix(String8 str, u64 size);
GROUNDED_FUNCTION String8 str8Skip(String8 str, u64 amount);
GROUNDED_FUNCTION String8 str8Substring(String8 str, u64 first, u64 last);
GROUNDED_FUNCTION bool str8IsEqual(String8 a, String8 b);
GROUNDED_FUNCTION int str8Compare(String8 a, String8 b);
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

GROUNDED_FUNCTION String8 str8ReplaceCharacter(struct MemoryArena* arena, String8 str, u8 old, u8 new);
// Removes all occurences of character from str and copies it into new 0-terminated output string
GROUNDED_FUNCTION String8 str8RemoveCharacter(struct MemoryArena* arena, String8 str, u8 character);

GROUNDED_FUNCTION void str8ListPushExplicit(String8List* list, String8 str, String8Node* nodeMemory);
GROUNDED_FUNCTION void str8ListPush(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION void str8ListPushCopy(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION void str8ListPushCopyAndNullTerminate(struct MemoryArena* arena, String8List* list, String8 str);
GROUNDED_FUNCTION String8 str8ListJoin(struct MemoryArena* arena, String8List* list, StringJoin* optionalJoin);
GROUNDED_FUNCTION String8List str8Split(struct MemoryArena* arena, String8 str, u8* splitCharacters, u64 count);

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
GROUNDED_FUNCTION StringDecode strDecodeUtf8(u8* string, u32 capacity); // result.size is the number of bytes that have been advanced
// dst must have at least 4 bytes of space available
GROUNDED_FUNCTION u32 strEncodeUtf8(u8* dst, u32 codepoint); // Returns bytes advanced in dst. Maximum possible advance is 4 bytes.
// Capacity is the number of u16 left. In error case result.size is 0
GROUNDED_FUNCTION StringDecode strDecodeUtf16(u16* string, u32 capacity); // result.size is the number of u16 that have been advanced
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

GROUNDED_FUNCTION_INLINE bool compareAtoms(StringAtom* a0, StringAtom* a1) {
    if(a0->hash == a1->hash) {
        // Hash the same so check the string to make sure
        return str8IsEqual(a0->string, a1->string);
    }
    return false;
}

#endif // GROUNDED_STRING_H
