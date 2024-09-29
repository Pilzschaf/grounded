#ifndef GROUNDED_H
#define GROUNDED_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdalign.h>
#include <stddef.h> // For offsetof

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// For better support of GLSL
typedef unsigned int uint;

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(a) (sizeof(a)/sizeof(*(a)))
#endif
#ifndef PASS_ARRAY
#define PASS_ARRAY(a) ARRAY_COUNT(a), a
#endif

#ifndef OFFSET_OF_MEMBER
#define OFFSET_OF_MEMBER(T, m) (offsetof(T, m))
#endif

#ifndef ALIGNMENT_OF
#define ALIGNMENT_OF(T) (alignof(T))
#endif

#ifndef INT_FROM_PTR
#define INT_FROM_PTR(p) (unsigned long long)(((char*)p) - (char*)0)
#endif
#ifndef PTR_FROM_INT
#define PTR_FROM_INT(n) (void*)((char*)0 + (n))
#endif

#ifndef STRINGIFY_
#define STRINGIFY_(S) #S
#endif
#ifndef STRINGIFY
#define STRINGIFY(S) STRINGIFY_(S)
#endif
#ifndef GLUE_
#define GLUE_(A, B) A##B
#endif
#ifndef GLUE
#define GLUE(A, B) GLUE_(A, B)
#endif

#ifndef ASSERT
#define ASSERT(c) assert(c)
#endif
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(c) enum { GLUE(assert_fail_, __LINE__) = 1/(int)(!!(c))}
#endif

// Check type sizes
STATIC_ASSERT(sizeof(s8) == 1);
STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(s16) == 2);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(s32) == 4);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(s64) == 8);
STATIC_ASSERT(sizeof(u64) == 8);
STATIC_ASSERT(sizeof(uint) == 4);
STATIC_ASSERT(sizeof(int) == 4);
STATIC_ASSERT(sizeof(bool) == 1);
STATIC_ASSERT(sizeof(float) == 4);
STATIC_ASSERT(sizeof(double) == 8);
STATIC_ASSERT(sizeof(void*) == 8);

#ifndef GROUNDED_EXTERN_C_SPECIFIER
    #ifdef __cplusplus
    #define GROUNDED_EXTERN_C_SPECIFIER extern "C"
    #else
    #define GROUNDED_EXTERN_C_SPECIFIER 
    #endif
#endif

#ifndef GROUNDED_API
    #ifdef _WIN32
        #if defined(GROUNDED_WIN32_DYNAMIC_EXPORT)
        #define GROUNDED_API __declspec(dllexport)
        #elif defined(GROUNDED_WIN32_DYNAMIC_IMPORT)
        #define GROUNDED_API __declspec(dllimport)
        #endif
    #endif
    #ifndef GROUNDED_API
    #define GROUNDED_API
    #endif
#endif

// GROUNDED_FUNCTION should be used for all grounded functions that are not defined in header files
// GROUNDED_FUNCTION_INLINE should be used for all grounded functions that are defined in header files
#ifdef GROUNDED_SINGLE_COMPILATION_UNIT
    #ifndef GROUNDED_FUNCTION
    #define GROUNDED_FUNCTION GROUNDED_EXTERN_C_SPECIFIER static
    #endif
    #ifndef GROUNDED_FUNCTION_INLINE
    #define GROUNDED_FUNCTION_INLINE GROUNDED_EXTERN_C_SPECIFIER static inline
    #endif
#else
    #ifndef GROUNDED_FUNCTION
    #define GROUNDED_FUNCTION GROUNDED_EXTERN_C_SPECIFIER GROUNDED_API
    #endif
    
    #ifndef GROUNDED_FUNCTION_INLINE
    #define GROUNDED_FUNCTION_INLINE static inline
    #endif
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef CLAMP
#define CLAMP(a,x,b) (((x)<(a))?(a):(((b)<(x))?(b):(x)))
#endif
#ifndef CLAMP_TOP
#define CLAMP_TOP(a,b) MIN(a,b)
#endif
#ifndef CLAMP_BOT
#define CLAMP_BOT(a,b) MAX(a,b)
#endif
#ifndef ALIGN_UP_POW2
#define ALIGN_UP_POW2(x, p) (((x)+(p) - 1) &~((p) - 1))
#endif
#ifndef ALIGN_DOWN_POW2
#define ALIGN_DOWN_POW2(x, p) ((x)&~((p) - 1))
#endif
#ifndef IS_POW2
#define IS_POW2(x) (((x) != 0) && !((x) & ((x) - 1)))
#endif
#ifndef IS_POW2_ALLOW_ZERO
#define IS_POW2_ALLOW_ZERO(x) (!((x) & ((x) - 1)))
#endif
#ifndef IS_MASK // true if number contains a single continous stream of 1s. Does not include 0
#define IS_MASK(x) (((x) != 0) && ((x) | ((x) - 1)))
#endif
#ifndef IS_MASK_ALLOW_ZERO // true if number contains a single continous stream of 1s. Does include 0
#define IS_MASK_ALLOW_ZERO(x) ((x) | ((x) - 1))
#endif
#ifndef SQUARE
#define SQUARE(x) ((x)*(x))
#endif
#ifndef CUBE
#define CUBE(x) ((x)*(x)*(x))
#endif
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#ifndef KB
#define KB(x) ((x) << 10)
#endif
#ifndef MB
#define MB(x) ((x) << 20)
#endif
#ifndef GB
#define GB(x) ((x) << 30)
#endif
#ifndef TB
#define TB(x) ((x) << 40)
#endif

#ifndef PI32
#define PI32 (3.14159265359f)
#endif
#ifndef PI64
#define PI64 (3.14159265358979323846)
#endif


//////////
// Strings

// String data should always be handled as being immutable.
// Base does not guarantee to have a null-terminator. Always use str8GetCstr if a C-String is required
// String data should always be UTF-8
typedef struct {
    u8* base;
    u64 size;
} String8;

typedef struct String8Node {
    struct String8Node* next;
    String8 string;
} String8Node;

typedef struct {
    struct String8Node* first;
    struct String8Node* last;
    u64 numNodes;
    u64 totalSize;
} String8List;

typedef struct {
    String8 pre;
    String8 mid;
    String8 post;
} StringJoin;

typedef struct {
    u16* base;
    u64 size; // Number of u16
} String16;

typedef struct {
    u32* base;
    u64 size; // Number of u32
} String32;

typedef struct {
    u32 codepoint;
    u32 size; // Size of this codepoint in the source string
} StringDecode;

// An atom contains a hash which can be used for fast comparisons.
// Equality check always requires a string compare if the hash matches
// Simply use compareAtoms
typedef struct {
    String8 string;
    u64 hash;
} StringAtom;

#ifndef __cplusplus
#define STR8_LITERAL(s) ((String8){(u8*)(s), sizeof(s) - 1})
#define EMPTY_STRING8 ((String8){0, 0})
#else
#define STR8_LITERAL(s) (String8{(u8*)(s), sizeof(s) - 1})
#define EMPTY_STRING8 (String8{0, 0})
#endif

GROUNDED_FUNCTION_INLINE u32 groundedNextPow2u32(u32 value) {
    ASSERT(value <= INT32_MAX);
    value = value - 1;
    value = value | (value >> 1);
    value = value | (value >> 2);
    value = value | (value >> 4);
    value = value | (value >> 8);
    value = value | (value >> 16);
    return value + 1;
}

////////////////
// Scratch arena

// If the current function already uses an arena for persisting allocations, it should be passed as conflictArena
// The returned arena should be used for temporary allocations that are reset in the same scope (temporary stack based allocations)
GROUNDED_FUNCTION struct MemoryArena* threadContextGetScratch(struct MemoryArena* conflictArena);


/////////
// Errors

typedef struct GroundedError {
    String8 text;
    u64 line;
    String8 filename;
} GroundedError;

#ifndef GROUNDED_PUSH_ERROR
#define GROUNDED_PUSH_ERROR(str) groundedPushError(str8FromCstr(str), STR8_LITERAL(__FILE__), __LINE__)
#endif
#ifndef GROUNDED_PUSH_ERRORF
#define GROUNDED_PUSH_ERRORF(str, ...) groundedPushErrorf(STR8_LITERAL(__FILE__), __LINE__, str, __VA_ARGS__)
#endif
#ifndef GROUNDED_PUSH_ERROR_STR8
#define GROUNDED_PUSH_ERROR_STR8(str) groundedPushError(str, STR8_LITERAL(__FILE__), __LINE__)
#endif
GROUNDED_FUNCTION void groundedPushError(String8 str, String8 filename, u64 line);
GROUNDED_FUNCTION void groundedPushErrorf(String8 filename, u64 line, const char* fmt, ...);
GROUNDED_FUNCTION bool groundedHasError();
GROUNDED_FUNCTION GroundedError* groundedPopError();

#endif // GROUNDED_H
