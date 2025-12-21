#ifndef GROUNDED_MATH_H
#define GROUNDED_MATH_H

#include "../grounded.h"

#ifndef GROUNDED_MATH_PREFIX
#define GROUNDED_MATH_PREFIX(name) name
#endif

#ifndef VEC2
#define VEC2(x, y) ((GROUNDED_MATH_PREFIX(vec2)){{(x), (y)}})
#endif
#ifndef UVEC2
#define UVEC2(x, y) ((GROUNDED_MATH_PREFIX(uvec2)){{x, y}})
#endif
#ifndef IVEC2
#define IVEC2(x, y) ((GROUNDED_MATH_PREFIX(ivec2)){{x, y}})
#endif

#ifndef VEC3
#define VEC3(x, y, z) ((GROUNDED_MATH_PREFIX(vec3)){{x, y, z}})
#endif
#ifndef UVEC3
#define UVEC3(x, y, z) ((GROUNDED_MATH_PREFIX(uvev3)){{x, y, z}})
#endif
#ifndef IVEC3
#define IVEC3(x, y, z) ((GROUNDED_MATH_PREFIX(ivec3)){{x, y, z}})
#endif

#ifndef VEC4
#define VEC4(x, y, z, w) ((GROUNDED_MATH_PREFIX(vec4)){{x, y, z, w}})
#endif
#ifndef UVEC4
#define UVEC4(x, y, z, w) ((GROUNDED_MATH_PREFIX(uvec4)){{x, y, z, w}})
#endif
#ifndef IVEC4
#define IVEC4(x, y, z, w) ((GROUNDED_MATH_PREFIX(ivec4)){{x, y, z, w}})
#endif

typedef union GROUNDED_MATH_PREFIX(vec2) {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
    float elements[2];
} GROUNDED_MATH_PREFIX(vec2);

typedef union GROUNDED_MATH_PREFIX(uvec2) {
    struct {
        u32 x, y;
    };
    struct {
        u32 width, height;
    };
    u32 elements[2];
} GROUNDED_MATH_PREFIX(uvec2);

typedef union GROUNDED_MATH_PREFIX(ivec2) {
    struct {
        s32 x, y;
    };
    struct {
        s32 width, height;
    };
    s32 elements[2];
} GROUNDED_MATH_PREFIX(ivec2);

typedef union GROUNDED_MATH_PREFIX(vec3) {
    struct {
        float x, y, z;
    };
    struct {
        float u, v, w;
    };
    GROUNDED_MATH_PREFIX(vec2) xy;
    GROUNDED_MATH_PREFIX(vec2) uv;
    float elements[3];
} GROUNDED_MATH_PREFIX(vec3);

typedef union GROUNDED_MATH_PREFIX(uvev3) {
    struct {
        u32 x, y, z;
    };
    struct {
        u32 width, height, depth;
    };
    u32 elements[3];
} GROUNDED_MATH_PREFIX(uvev3);

typedef union GROUNDED_MATH_PREFIX(ivec3) {
    struct {
        s32 x, y, z;
    };
    struct {
        s32 width, height, depth;
    };
    s32 elements[3];
} GROUNDED_MATH_PREFIX(ivec3);

typedef union GROUNDED_MATH_PREFIX(vec4) {
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };
    struct {
        GROUNDED_MATH_PREFIX(vec3) xyz;
    };
    GROUNDED_MATH_PREFIX(vec3) rgb;
    GROUNDED_MATH_PREFIX(vec2) xy;
    float elements[4];
} GROUNDED_MATH_PREFIX(vec4);

typedef union GROUNDED_MATH_PREFIX(uvec4) {
    struct {
        u32 x, y, z, w;
    };
    u32 elements[4];
} GROUNDED_MATH_PREFIX(uvec4);

typedef union GROUNDED_MATH_PREFIX(ivec4) {
    struct {
        s32 x, y, z, w;
    };
    s32 elements[4];
} GROUNDED_MATH_PREFIX(ivec4);

typedef union GROUNDED_MATH_PREFIX(quat) {
    struct {
        float x, y, z, w;
    };
    GROUNDED_MATH_PREFIX(vec3) xyz;
    GROUNDED_MATH_PREFIX(vec4) asVec4;
    float elements[4];
} GROUNDED_MATH_PREFIX(quat);

typedef union GROUNDED_MATH_PREFIX(mat2) {
    float elements[4];
    struct {
        float   m11, m12,
                m21, m22;
    };
    float m[2][2];
    GROUNDED_MATH_PREFIX(vec2) rows[2];
} GROUNDED_MATH_PREFIX(mat2);
STATIC_ASSERT(sizeof(GROUNDED_MATH_PREFIX(mat2)) == sizeof(float)*4);

//TODO: Specify matrix multiplication order and storage order
typedef union GROUNDED_MATH_PREFIX(mat4) {
    float elements[16];
    struct {
        float   m11, m12, m13, m14,
                m21, m22, m23, m24,
                m31, m32, m33, m34,
                m41, m42, m43, m44;
    };
    // Row major: m[row][column]
    float m[4][4];
    GROUNDED_MATH_PREFIX(vec4) rows[4];
} GROUNDED_MATH_PREFIX(mat4);
STATIC_ASSERT(sizeof(GROUNDED_MATH_PREFIX(mat4)) == sizeof(float)*16);

typedef struct GROUNDED_MATH_PREFIX(frustum) {
    vec4 frustumPlanes[6];
} GROUNDED_MATH_PREFIX(frustum);

#include <math.h> // For sqrtf, sinf, cosf, tanf

GROUNDED_FUNCTION_INLINE float squareRoot(float value) {
    float result = sqrtf(value);
    return result;
}

GROUNDED_FUNCTION_INLINE float lerp(float a, float f, float b) {
    return (1.0f - f)*a + f*b;
}

GROUNDED_FUNCTION_INLINE float unlerp(float a, float f, float b) {
    float t = 0.0f;
    if(a != b) {
        t = (f - a)/(b - a);
    }
    return t;
}

GROUNDED_FUNCTION_INLINE float signOf(float value) {
    return value >= 0.0f ? 1.0f : -1.0f;
}

//////////
// Vector2
GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Splat(float v) {
    GROUNDED_MATH_PREFIX(vec2) result = VEC2(v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2AddScalar(GROUNDED_MATH_PREFIX(vec2) v, float c) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = v.x + c;
    result.y = v.y + c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Add(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2SubtractScalar(GROUNDED_MATH_PREFIX(vec2) v, float c) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = v.x - c;
    result.y = v.y - c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Subtract(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Flip(GROUNDED_MATH_PREFIX(vec2) v) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2MultiplyScalar(GROUNDED_MATH_PREFIX(vec2) v, float c) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = v.x * c;
    result.y = v.y * c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2DivideScalar(GROUNDED_MATH_PREFIX(vec2) v, float c) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = v.x / c;
    result.y = v.y / c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Hadamard(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

GROUNDED_FUNCTION_INLINE float v2Dot(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    float result = a.x * b.x + a.y * b.y;
    return result;
}

GROUNDED_FUNCTION_INLINE float v2LengthSq(GROUNDED_MATH_PREFIX(vec2) v) {
    float result = v2Dot(v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE float v2Length(GROUNDED_MATH_PREFIX(vec2) v) {
    float result = squareRoot(v2LengthSq(v));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Normalize(GROUNDED_MATH_PREFIX(vec2) v) {
    float l = v2Length(v);
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = v.x / l;
    result.y = v.y / l;
    return result;
}

GROUNDED_FUNCTION_INLINE float v2DistanceSq(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    float result = v2LengthSq(v2Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE float v2Distance(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    float result = v2Length(v2Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Lerp(GROUNDED_MATH_PREFIX(vec2) a, float f, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = lerp(a.x, f, b.x);
    result.y = lerp(a.y, f, b.y);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Perp(GROUNDED_MATH_PREFIX(vec2) v) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = -v.y;
    result.y = v.x;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Abs(GROUNDED_MATH_PREFIX(vec2) v) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = ABS(v.x);
    result.y = ABS(v.y);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Max(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2Min(GROUNDED_MATH_PREFIX(vec2) a, GROUNDED_MATH_PREFIX(vec2) b) {
    GROUNDED_MATH_PREFIX(vec2) result;
    result.x = MIN(a.x, b.x);
    result.y = MIN(a.y, b.y);
    return result;
}

//////////
// Vector3
GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Splat(float v) {
    GROUNDED_MATH_PREFIX(vec3) result = VEC3(v, v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3AddScalar(GROUNDED_MATH_PREFIX(vec3) v, float c) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = v.x + c;
    result.y = v.y + c;
    result.z = v.z + c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Add(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3SubtractScalar(GROUNDED_MATH_PREFIX(vec3) v, float c) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = v.x - c;
    result.y = v.y - c;
    result.z = v.z - c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Subtract(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Flip(GROUNDED_MATH_PREFIX(vec3) v) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3MultiplyScalar(GROUNDED_MATH_PREFIX(vec3) v, float c) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3DivideScalar(GROUNDED_MATH_PREFIX(vec3) v, float c) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = v.x / c;
    result.y = v.y / c;
    result.z = v.z / c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Hadamard(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

GROUNDED_FUNCTION_INLINE float v3Dot(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    float result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Cross(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

GROUNDED_FUNCTION_INLINE float v3LengthSq(GROUNDED_MATH_PREFIX(vec3) v) {
    float result = v3Dot(v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE float v3Length(GROUNDED_MATH_PREFIX(vec3) v) {
    float result = squareRoot(v3LengthSq(v));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Normalize(GROUNDED_MATH_PREFIX(vec3) v) {
    GROUNDED_MATH_PREFIX(vec3) result;
    float l = v3Length(v);
    result.x = v.x / l;
    result.y = v.y / l;
    result.z = v.z / l;
    return result;
}

GROUNDED_FUNCTION_INLINE float v3DistanceSq(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    float result = v3LengthSq(v3Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE float v3Distance(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    float result = v3Length(v3Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Lerp(GROUNDED_MATH_PREFIX(vec3) a, float f, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = lerp(a.x, f, b.x);
    result.y = lerp(a.y, f, b.y);
    result.z = lerp(a.z, f, b.z);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Abs(GROUNDED_MATH_PREFIX(vec3) v) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = ABS(v.x);
    result.y = ABS(v.y);
    result.z = ABS(v.z);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Max(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    result.z = MAX(a.z, b.z);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) v3Min(GROUNDED_MATH_PREFIX(vec3) a, GROUNDED_MATH_PREFIX(vec3) b) {
    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = MIN(a.x, b.x);
    result.y = MIN(a.y, b.y);
    result.z = MIN(a.z, b.z);
    return result;
}

//////////
// Vector4
GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Splat(float v) {
    GROUNDED_MATH_PREFIX(vec4) result = VEC4(v, v, v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4AddScalar(GROUNDED_MATH_PREFIX(vec4) v, float c) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = v.x + c;
    result.y = v.y + c;
    result.z = v.z + c;
    result.w = v.w + c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Add(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4SubtractScalar(GROUNDED_MATH_PREFIX(vec4) v, float c) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = v.x - c;
    result.y = v.y - c;
    result.z = v.z - c;
    result.w = v.w - c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Subtract(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Flip(GROUNDED_MATH_PREFIX(vec4) v) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4MultiplyScalar(GROUNDED_MATH_PREFIX(vec4) v, float c) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = v.x * c;
    result.y = v.y * c;
    result.z = v.z * c;
    result.w = v.w * c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4DivideScalar(GROUNDED_MATH_PREFIX(vec4) v, float c) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = v.x / c;
    result.y = v.y / c;
    result.z = v.z / c;
    result.w = v.w / c;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Hadamard(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

GROUNDED_FUNCTION_INLINE float v4Dot(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    float result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result;
}

GROUNDED_FUNCTION_INLINE float v4LengthSq(GROUNDED_MATH_PREFIX(vec4) v) {
    float result = v4Dot(v, v);
    return result;
}

GROUNDED_FUNCTION_INLINE float v4Length(GROUNDED_MATH_PREFIX(vec4) v) {
    float result = squareRoot(v4LengthSq(v));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Normalize(GROUNDED_MATH_PREFIX(vec4) v) {
    GROUNDED_MATH_PREFIX(vec4) result;
    float l = v4Length(v);
    result.x = v.x / l;
    result.y = v.y / l;
    result.z = v.z / l;
    result.w = v.w / l;
    return result;
}

GROUNDED_FUNCTION_INLINE float v4DistanceSq(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    float result = v4LengthSq(v4Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE float v4Distance(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    float result = v4Length(v4Subtract(a, b));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Lerp(GROUNDED_MATH_PREFIX(vec4) a, float f, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = lerp(a.x, f, b.x);
    result.y = lerp(a.y, f, b.y);
    result.z = lerp(a.z, f, b.z);
    result.w = lerp(a.w, f, b.w);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Abs(GROUNDED_MATH_PREFIX(vec4) v) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = ABS(v.x);
    result.y = ABS(v.y);
    result.z = ABS(v.z);
    result.w = ABS(v.w);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Max(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    result.z = MAX(a.z, b.z);
    result.w = MAX(a.w, b.w);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) v4Min(GROUNDED_MATH_PREFIX(vec4) a, GROUNDED_MATH_PREFIX(vec4) b) {
    GROUNDED_MATH_PREFIX(vec4) result;
    result.x = MIN(a.x, b.x);
    result.y = MIN(a.y, b.y);
    result.z = MIN(a.z, b.z);
    result.w = MIN(a.w, b.w);
    return result;
}

/////////////
// Quaternion
// TODO: 
// Get angle
// Get rotation axis
// quaternion lerp
// quaternion slerp
// https://github.com/microsoft/DirectXMath/blob/549b51d9cc328903739d5fbe7f348aec00c83339/Inc/DirectXMathMisc.inl

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatCreateEuler(GROUNDED_MATH_PREFIX(vec3) eulerAnglesInRad) {
    GROUNDED_MATH_PREFIX(quat) result;
    GROUNDED_MATH_PREFIX(vec3) c;
    c.x = cosf(eulerAnglesInRad.x * 0.5f);
    c.y = cosf(eulerAnglesInRad.y * 0.5f);
    c.z = cosf(eulerAnglesInRad.y * 0.5f);
    GROUNDED_MATH_PREFIX(vec3) s;
    s.x = sinf(eulerAnglesInRad.x * 0.5f);
    s.y = sinf(eulerAnglesInRad.y * 0.5f);
    s.z = sinf(eulerAnglesInRad.z * 0.5f);

    result.w = c.x * c.y * c.z + s.x * s.y * s.z;
    result.x = s.x * c.y * c.z - c.x * s.y * s.z;
    result.y = c.x * s.y * c.z + s.x * c.y * s.z;
    result.z = c.y * c.y * s.z - s.y * s.y * c.z;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatCreateWithAxis(GROUNDED_MATH_PREFIX(vec3) axis, float angleInRad) {
    axis = v3Normalize(axis);
    axis = v3MultiplyScalar(axis, sinf(angleInRad*0.5f));
    GROUNDED_MATH_PREFIX(quat) result = {{axis.x, axis.y, axis.z, cosf(angleInRad*0.5f)}};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatCreateIdentity() {
    GROUNDED_MATH_PREFIX(quat) result = {{0.0f, 0.0f, 0.0f, 1.0f}};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatFromVec4(GROUNDED_MATH_PREFIX(vec4) v) {
    GROUNDED_MATH_PREFIX(quat) result;
    result.asVec4 = v;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) quatMultiplyV3(GROUNDED_MATH_PREFIX(quat) q, GROUNDED_MATH_PREFIX(vec3) v) {
    float tmpX, tmpY, tmpZ, tmpW;
    tmpX = (((q.w * v.x) + (q.y * v.z)) - (q.z * v.y));
    tmpY = (((q.w * v.y) + (q.z * v.x)) - (q.x * v.z));
    tmpZ = (((q.w * v.z) + (q.x * v.y)) - (q.y * v.x));
    tmpW = (((q.x * v.x) + (q.y * v.y)) + (q.z * v.z));

    GROUNDED_MATH_PREFIX(vec3) result;
    result.x = ((((tmpW * q.x) + (tmpX * q.w)) - (tmpY * q.z)) + (tmpZ * q.y));
    result.y = ((((tmpW * q.y) + (tmpY * q.w)) - (tmpZ * q.x)) + (tmpX * q.z));
    result.z = ((((tmpW * q.z) + (tmpZ * q.w)) - (tmpX * q.y)) + (tmpY * q.x));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatMultiplyQuat(GROUNDED_MATH_PREFIX(quat) a, GROUNDED_MATH_PREFIX(quat) b) {
    GROUNDED_MATH_PREFIX(quat) result;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.z * b.y - a.y * b.x + a.z * b.w;
    return result;
}

GROUNDED_FUNCTION_INLINE float quatLengthSq(GROUNDED_MATH_PREFIX(quat) q) {
    float result = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    return result;
}

GROUNDED_FUNCTION_INLINE float quatLength(GROUNDED_MATH_PREFIX(quat) q) {
    float result = squareRoot(quatLengthSq(q));
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatNormalize(GROUNDED_MATH_PREFIX(quat) q) {
    float l = quatLength(q);
    GROUNDED_MATH_PREFIX(quat) result;
    result.x = q.x / l;
    result.y = q.y / l;
    result.z = q.z / l;
    result.w = q.w / l;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatDivide(GROUNDED_MATH_PREFIX(quat) q, float dividend) {
    GROUNDED_MATH_PREFIX(quat) result;
    result.x = q.x / dividend;
    result.y = q.y / dividend;
    result.z = q.z / dividend;
    result.w = q.w / dividend;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(quat) quatInverse(GROUNDED_MATH_PREFIX(quat) q) {
    GROUNDED_MATH_PREFIX(quat) result;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) quatToMat(GROUNDED_MATH_PREFIX(quat) q) {
    GROUNDED_MATH_PREFIX(mat4) result;
    float qxx = q.x * q.x;
    float qyy = q.y * q.y;
    float qzz = q.z * q.z;
    float qxy = q.x * q.y;
    float qwz = q.w * q.z;
    float qxz = q.x * q.z;
    float qwy = q.w * q.y;
    float qyz = q.y * q.z;
    float qwx = q.w * q.x;

    result.m11 = 1.0f - 2.0f * qyy - 2.0f * qzz;
    result.m12 = 2.0f * qxy + 2.0f * qwz;
    result.m13 = 2.0f * qxz - 2.0f * qwy;
    result.m14 = 0.0f;
    
    result.m21 = 2.0f * qxy - 2.0f * qwz;
    result.m22 = 1.0f - 2.0f * qxx - 2.0f * qzz;
    result.m23 = 2.0f * qyz + 2.0f * qwx;
    result.m24 = 0.0f;
    
    result.m31 = 2.0f * qxz + 2.0f * qwy;
    result.m32 = 2.0f * qyz - 2.0f * qwx;
    result.m33 = 1.0f - 2.0f * qxx - 2.0f * qyy;
    result.m34 = 0.0f;
    
    result.m41 = 0.0f;
    result.m42 = 0.0f;
    result.m43 = 0.0f;
    result.m44 = 1.0f;
    
    return result;
}

//////////
// Matrix2
GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2CreateIdentity() {
    GROUNDED_MATH_PREFIX(mat2) result = {{
        1, 0,
        0, 1
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2CreateScaleMatrix(GROUNDED_MATH_PREFIX(vec2) scale) {
    GROUNDED_MATH_PREFIX(mat2) result = {{
        scale.x, 0.0f,
        0.0f, scale.y,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2CreateRotationMatrix(float angleInRad) {
    float c = cosf(angleInRad);
    float s = sinf(angleInRad);

    GROUNDED_MATH_PREFIX(mat2) result = {{
        c,    -s,
        s,    c,
    }};

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2Multiply( GROUNDED_MATH_PREFIX(mat2) a,  GROUNDED_MATH_PREFIX(mat2) b) {
    GROUNDED_MATH_PREFIX(mat2) result = {{
        b.m11 * a.m11 + b.m21 * a.m12,
        b.m12 * a.m11 + b.m22 * a.m12,
        b.m11 * a.m21 + b.m21 * a.m22,
        b.m12 * a.m21 + b.m22 * a.m22,
    }};

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2MultiplyScalar(GROUNDED_MATH_PREFIX(mat2) m, float c) {
    GROUNDED_MATH_PREFIX(mat2) result;
    result.rows[0] = v2MultiplyScalar(m.rows[0], c);
    result.rows[1] = v2MultiplyScalar(m.rows[1], c);

    return result;    
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) mat2MultiplyVec2(GROUNDED_MATH_PREFIX(mat2) m, GROUNDED_MATH_PREFIX(vec2) v) {
    GROUNDED_MATH_PREFIX(vec2) result = {{
        m.m11 * v.x + m.m12 * v.y,
        m.m21 * v.x + m.m22 * v.y,
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2Transpose(GROUNDED_MATH_PREFIX(mat2) m) {
    GROUNDED_MATH_PREFIX(mat2) result = {{
        m.m11, m.m21,
        m.m12, m.m22,
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) mat2GetRow(GROUNDED_MATH_PREFIX(mat2) m, u32 index) {
    ASSERT(index < 2);
    GROUNDED_MATH_PREFIX(vec2) result = m.rows[index];
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) mat2GetColumn(GROUNDED_MATH_PREFIX(mat2) m, u32 index) {
    ASSERT(index < 2);
    GROUNDED_MATH_PREFIX(vec2) result = {{
        m.m[0][index],
        m.m[1][index],
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat2) mat2Inverse(GROUNDED_MATH_PREFIX(mat2) m) {
    GROUNDED_MATH_PREFIX(mat2) result = m;

    float determinant = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
    if(determinant != 0.0f) {
        float invDet = 1.0f / determinant;
        result.m[0][0] =  m.m[1][1] * invDet;
        result.m[0][1] = -m.m[0][1] * invDet;
        result.m[1][0] = -m.m[1][0] * invDet;
        result.m[1][1] =  m.m[0][0] * invDet;
    } else {
        // Matrix is not invertible.
        ASSERT(false);
    }

    return result;
}

//////////
// Matrix4
GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateIdentity() {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreatePerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    float tanHalfFov = tanf(fov / 2.0f);
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
    result.m[1][1] = -1.0f / tanHalfFov; // the - is a negative y scale and makes y go up instead of down
    result.m[2][2] = farPlane / (farPlane - nearPlane);
    result.m[2][3] = -(farPlane * nearPlane) / (farPlane - nearPlane);
    result.m[3][2] = -1.0f;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreatePerspectiveProjectionGl(float fov, float aspectRatio, float nearPlane, float farPlane) {
    float tanHalfFov = tanf(fov / 2.0f);
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
    result.m[1][1] = 1.0f / tanHalfFov; // the - is a negative y scale and makes y go up instead of down
    result.m[2][2] = -farPlane / (farPlane - nearPlane);
    result.m[2][3] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    result.m[3][2] = -1.0f;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreatePerspectiveProjectionInverseZ(float fov, float aspectRatio, float nearPlane) {
    float tanHalfFov = tanf(fov / 2.0f);
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
    result.m[1][1] = -1.0f / tanHalfFov; // the - is a negative y scale and makes y go up instead of down
    result.m[2][3] = nearPlane;
    result.m[3][2] = 1.0f;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreatePerspectiveProjectionFromCoordinates(float left, float right, float bottom, float top, float zNear, float zFar) {
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    /*result.m11 = 2.0f * zNear / (right - left);
    result.m22 = 2.0f * zNear / (top - bottom);
    result.m31 = (right + left) / (right - left);
    result.m32 = (top + bottom) / (top - bottom);
    result.m33 = (zFar + zNear) / (zFar - zNear);
    result.m34 = 1.0f;
    result.m43 = -2.0f * zFar * zNear / (zFar - zNear);*/

    result.m11 = 2.0f * zNear / (right - left);
    result.m22 = 2.0f * zNear / (top - bottom);
    result.m31 = (right + left) / (right - left);
    result.m32 = (top + bottom) / (top - bottom);
    result.m33 = -(zFar + zNear) / (zFar - zNear);
    result.m43 = -1.0f;
    result.m34 = -2.0f * zFar * zNear / (zFar - zNear);

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateOrthographicProjection(float aspectRatio, float nearPlane, float farPlane) {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, aspectRatio, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f / (nearPlane - farPlane), (nearPlane + farPlane) / (nearPlane - farPlane),
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateOrthographicProjectionFromCoordinates(float left, float right, float top, float bottom) {
    /*
    // OpenGL
    GROUNDED_MATH_PREFIX(mat4) result = {{
        2/(right - left), 0.0f, 0.0f, - (right + left) / (right - left),
        0.0f, 2/(top-bottom), 0.0f, -(top + bottom) / (top - bottom),
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};*/

    // Vulkan
    GROUNDED_MATH_PREFIX(mat4) result = {{
        2/(right - left), 0.0f, 0.0f, - (right + left) / (right - left),
        0.0f, -2/(top-bottom), 0.0f, (top + bottom) / (top - bottom),
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateTranslationMatrix(GROUNDED_MATH_PREFIX(vec3) translation) {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        1.0f, 0.0f, 0.0f, translation.x,
        0.0f, 1.0f, 0.0f, translation.y,
        0.0f, 0.0f, 1.0f, translation.z,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateScaleMatrix(GROUNDED_MATH_PREFIX(vec3) scale) {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateXRotationMatrix(float angleInRad) {
    float c = cosf(angleInRad);
    float s = sinf(angleInRad);

    GROUNDED_MATH_PREFIX(mat4) result = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,    c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateYRotationMatrix(float angleInRad) {
    float c = cosf(angleInRad);
    float s = sinf(angleInRad);

    GROUNDED_MATH_PREFIX(mat4) result = {{
        c,    0.0f, s,    0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s,   0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateZRotationMatrix(float angleInRad) {
    float c = cosf(angleInRad);
    float s = sinf(angleInRad);

    GROUNDED_MATH_PREFIX(mat4) result = {{
        c,    -s,   0.0f, 0.0f,
        s,    c,    0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
    
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateRotationMatrix(GROUNDED_MATH_PREFIX(vec3) axis, float angleInRad) {
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    float c = cosf(angleInRad);
    float s = sinf(angleInRad);
    axis = v3Normalize(axis);
    GROUNDED_MATH_PREFIX(vec3) temp = v3MultiplyScalar(axis, 1.0f - c);

    result.m[0][0] = c + temp.x * axis.x;
    result.m[1][0] = temp.x * axis.y + s * axis.z;
    result.m[2][0] = temp.x * axis.z - s * axis.y;

    result.m[0][1] = temp.y * axis.x - s * axis.z;
    result.m[1][1] = c + temp.y * axis.y;
    result.m[2][1] = temp.y * axis.z + s * axis.x;

    result.m[0][2] = temp.z * axis.x + s * axis.y;
    result.m[1][2] = temp.z * axis.y - s * axis.x;
    result.m[2][2] = c + temp.z * axis.z;

    result.m[3][3] = 1.0f;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matCreateLookAt(GROUNDED_MATH_PREFIX(vec3) eye, GROUNDED_MATH_PREFIX(vec3) center, GROUNDED_MATH_PREFIX(vec3) up) {
    GROUNDED_MATH_PREFIX(mat4) result = {0};

    GROUNDED_MATH_PREFIX(vec3) f = v3Normalize(v3Subtract(center, eye));
    GROUNDED_MATH_PREFIX(vec3) s = v3Cross(up, f);
    GROUNDED_MATH_PREFIX(vec3) u = v3Cross(f, s); 

    // Just a basis change matrix
    result.m[0][0] = s.x;
    result.m[0][1] = s.y;
    result.m[0][2] = s.z;
    result.m[1][0] = u.x;
    result.m[1][1] = u.y;
    result.m[1][2] = u.z;
    result.m[2][0] = f.x;
    result.m[2][1] = f.y;
    result.m[2][2] = f.z;

    // Translation
    result.m[0][3] = -v3Dot(s, eye);
    result.m[1][3] = -v3Dot(u, eye);
    result.m[2][3] = -v3Dot(f, eye);

    result.m[3][3] = 1.0f;

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matMultiply( GROUNDED_MATH_PREFIX(mat4) a,  GROUNDED_MATH_PREFIX(mat4) b) {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        b.m11 * a.m11 + b.m21 * a.m12 + b.m31 * a.m13 + b.m41 * a.m14,
        b.m12 * a.m11 + b.m22 * a.m12 + b.m32 * a.m13 + b.m42 * a.m14,
        b.m13 * a.m11 + b.m23 * a.m12 + b.m33 * a.m13 + b.m43 * a.m14,
        b.m14 * a.m11 + b.m24 * a.m12 + b.m34 * a.m13 + b.m44 * a.m14,
        b.m11 * a.m21 + b.m21 * a.m22 + b.m31 * a.m23 + b.m41 * a.m24,
        b.m12 * a.m21 + b.m22 * a.m22 + b.m32 * a.m23 + b.m42 * a.m24,
        b.m13 * a.m21 + b.m23 * a.m22 + b.m33 * a.m23 + b.m43 * a.m24,
        b.m14 * a.m21 + b.m24 * a.m22 + b.m34 * a.m23 + b.m44 * a.m24,
        b.m11 * a.m31 + b.m21 * a.m32 + b.m31 * a.m33 + b.m41 * a.m34,
        b.m12 * a.m31 + b.m22 * a.m32 + b.m32 * a.m33 + b.m42 * a.m34,
        b.m13 * a.m31 + b.m23 * a.m32 + b.m33 * a.m33 + b.m43 * a.m34,
        b.m14 * a.m31 + b.m24 * a.m32 + b.m34 * a.m33 + b.m44 * a.m34,
        b.m11 * a.m41 + b.m21 * a.m42 + b.m31 * a.m43 + b.m41 * a.m44,
        b.m12 * a.m41 + b.m22 * a.m42 + b.m32 * a.m43 + b.m42 * a.m44,
        b.m13 * a.m41 + b.m23 * a.m42 + b.m33 * a.m43 + b.m43 * a.m44,
        b.m14 * a.m41 + b.m24 * a.m42 + b.m34 * a.m43 + b.m44 * a.m44
    }};

    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matMultiplyScalar(GROUNDED_MATH_PREFIX(mat4) m, float c) {
    GROUNDED_MATH_PREFIX(mat4) result;
    result.rows[0] = v4MultiplyScalar(m.rows[0], c);
    result.rows[1] = v4MultiplyScalar(m.rows[1], c);
    result.rows[2] = v4MultiplyScalar(m.rows[2], c);
    result.rows[3] = v4MultiplyScalar(m.rows[3], c);

    return result;    
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) matMultiplyVec4(GROUNDED_MATH_PREFIX(mat4) m, GROUNDED_MATH_PREFIX(vec4) v) {
    GROUNDED_MATH_PREFIX(vec4) result = {{
        m.m11 * v.x + m.m12 * v.y + m.m13 * v.z + m.m14 * v.w,
        m.m21 * v.x + m.m22 * v.y + m.m23 * v.z + m.m24 * v.w,
        m.m31 * v.x + m.m32 * v.y + m.m33 * v.z + m.m34 * v.w,
        m.m41 * v.x + m.m42 * v.y + m.m43 * v.z + m.m44 * v.w,
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) matMultiplyVec3(GROUNDED_MATH_PREFIX(mat4) m, GROUNDED_MATH_PREFIX(vec3) v) {
    GROUNDED_MATH_PREFIX(vec4) temp;
    temp.xyz = v;
    temp.w = 1.0f;
    temp = matMultiplyVec4(m, temp);

    GROUNDED_MATH_PREFIX(vec3) result = {{
        temp.x / temp.w,
        temp.y / temp.w,
        temp.z / temp.w,
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matTranspose(GROUNDED_MATH_PREFIX(mat4) m) {
    GROUNDED_MATH_PREFIX(mat4) result = {{
        m.m11, m.m21, m.m31, m.m41,
        m.m12, m.m22, m.m32, m.m42,
        m.m13, m.m23, m.m33, m.m43,
        m.m14, m.m24, m.m34, m.m44,
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) matGetRow(GROUNDED_MATH_PREFIX(mat4) m, u32 index) {
    ASSERT(index < 4);
    GROUNDED_MATH_PREFIX(vec4) result = m.rows[index];
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) matGetColumn(GROUNDED_MATH_PREFIX(mat4) m, u32 index) {
    ASSERT(index < 4);
    GROUNDED_MATH_PREFIX(vec4) result = {{
        m.m[0][index],
        m.m[1][index],
        m.m[2][index],
        m.m[3][index],
    }};
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(mat4) matInverse(GROUNDED_MATH_PREFIX(mat4) m) {
    float coef00 = m.m[2][2] * m.m[3][3] - m.m[3][2] * m.m[2][3];
    float coef02 = m.m[1][2] * m.m[3][3] - m.m[3][2] * m.m[1][3];
    float coef03 = m.m[1][2] * m.m[2][3] - m.m[2][2] * m.m[1][3];

    float coef04 = m.m[2][1] * m.m[3][3] - m.m[3][1] * m.m[2][3];
    float coef06 = m.m[1][1] * m.m[3][3] - m.m[3][1] * m.m[1][3];
    float coef07 = m.m[1][1] * m.m[2][3] - m.m[2][1] * m.m[1][3];

    float coef08 = m.m[2][1] * m.m[3][2] - m.m[3][1] * m.m[2][2];
    float coef10 = m.m[1][1] * m.m[3][2] - m.m[3][1] * m.m[1][2];
    float coef11 = m.m[1][1] * m.m[2][2] - m.m[2][1] * m.m[1][2];

    float coef12 = m.m[2][0] * m.m[3][3] - m.m[3][0] * m.m[2][3];
    float coef14 = m.m[1][0] * m.m[3][3] - m.m[3][0] * m.m[1][3];
    float coef15 = m.m[1][0] * m.m[2][3] - m.m[2][0] * m.m[1][3];

    float coef16 = m.m[2][0] * m.m[3][2] - m.m[3][0] * m.m[2][2];
    float coef18 = m.m[1][0] * m.m[3][2] - m.m[3][0] * m.m[1][2];
    float coef19 = m.m[1][0] * m.m[2][2] - m.m[2][0] * m.m[1][2];

    float coef20 = m.m[2][0] * m.m[3][1] - m.m[3][0] * m.m[2][1];
    float coef22 = m.m[1][0] * m.m[3][1] - m.m[3][0] * m.m[1][1];
    float coef23 = m.m[1][0] * m.m[2][1] - m.m[2][0] * m.m[1][1];

    GROUNDED_MATH_PREFIX(vec4) fac0 = (GROUNDED_MATH_PREFIX(vec4)){{coef00, coef00, coef02, coef03}};
    GROUNDED_MATH_PREFIX(vec4) fac1 = (GROUNDED_MATH_PREFIX(vec4)){{coef04, coef04, coef06, coef07}};
    GROUNDED_MATH_PREFIX(vec4) fac2 = (GROUNDED_MATH_PREFIX(vec4)){{coef08, coef08, coef10, coef11}};
    GROUNDED_MATH_PREFIX(vec4) fac3 = (GROUNDED_MATH_PREFIX(vec4)){{coef12, coef12, coef14, coef15}};
    GROUNDED_MATH_PREFIX(vec4) fac4 = (GROUNDED_MATH_PREFIX(vec4)){{coef16, coef16, coef18, coef19}};
    GROUNDED_MATH_PREFIX(vec4) fac5 = (GROUNDED_MATH_PREFIX(vec4)){{coef20, coef20, coef22, coef23}};

    GROUNDED_MATH_PREFIX(vec4) tmpVec0 = (GROUNDED_MATH_PREFIX(vec4)){{m.m[1][0], m.m[0][0], m.m[0][0], m.m[0][0]}};
    GROUNDED_MATH_PREFIX(vec4) tmpVec1 = (GROUNDED_MATH_PREFIX(vec4)){{m.m[1][1], m.m[0][1], m.m[0][1], m.m[0][1]}};
    GROUNDED_MATH_PREFIX(vec4) tmpVec2 = (GROUNDED_MATH_PREFIX(vec4)){{m.m[1][2], m.m[0][2], m.m[0][2], m.m[0][2]}};
    GROUNDED_MATH_PREFIX(vec4) tmpVec3 = (GROUNDED_MATH_PREFIX(vec4)){{m.m[1][3], m.m[0][3], m.m[0][3], m.m[0][3]}};

    GROUNDED_MATH_PREFIX(vec4) inv0 = v4Add(v4Subtract(v4Hadamard(tmpVec1, fac0), v4Hadamard(tmpVec2, fac1)), v4Hadamard(tmpVec3, fac2));
    GROUNDED_MATH_PREFIX(vec4) inv1 = v4Add(v4Subtract(v4Hadamard(tmpVec0, fac0), v4Hadamard(tmpVec2, fac3)), v4Hadamard(tmpVec3, fac4));
    GROUNDED_MATH_PREFIX(vec4) inv2 = v4Add(v4Subtract(v4Hadamard(tmpVec0, fac1), v4Hadamard(tmpVec1, fac3)), v4Hadamard(tmpVec3, fac5));
    GROUNDED_MATH_PREFIX(vec4) inv3 = v4Add(v4Subtract(v4Hadamard(tmpVec0, fac2), v4Hadamard(tmpVec1, fac4)), v4Hadamard(tmpVec2, fac5));

    GROUNDED_MATH_PREFIX(vec4) signA = (GROUNDED_MATH_PREFIX(vec4)){{+1, -1, +1, -1}};
    GROUNDED_MATH_PREFIX(vec4) signB = (GROUNDED_MATH_PREFIX(vec4)){{-1, +1, -1, +1}};
    GROUNDED_MATH_PREFIX(mat4) result;
    result.rows[0] = v4Hadamard(inv0, signA);
    result.rows[1] = v4Hadamard(inv1, signB);
    result.rows[2] = v4Hadamard(inv2, signA);
    result.rows[3] = v4Hadamard(inv3, signB);

    GROUNDED_MATH_PREFIX(vec4) column0 = matGetColumn(result, 0);

    GROUNDED_MATH_PREFIX(vec4) dot0 = v4Hadamard(m.rows[0], column0);
    float dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

    float oneOverDeterminant = 1.0f / dot1;

    result = matMultiplyScalar(result, oneOverDeterminant);
    return result;
}

// Applies the inverse matrix transform to a vec3. Not working correctly
/*GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) matMultiplyVec3Inverse(GROUNDED_MATH_PREFIX(mat4) m, GROUNDED_MATH_PREFIX(vec3) v) {
    vec3 center = matMultiplyVec3(m, VEC3(0.0f, 0.0f, 0.0f));
    vec3 axisX = matMultiplyVec3(m, VEC3(1.0f, 0.0f, 0.0f));
    vec3 axisY = matMultiplyVec3(m, VEC3(0.0f, 1.0f, 0.0f));
    vec3 axisZ = matMultiplyVec3(m, VEC3(0.0f, 0.0f, 1.0f));
    axisX = (v3Subtract(axisX, center));
    axisY = (v3Subtract(axisY, center));
    axisZ = (v3Subtract(axisZ, center));
    axisX = v3Normalize(axisX);
    axisY = v3Normalize(axisY);
    axisZ = v3Normalize(axisZ);

    vec3 p = v3Subtract(v, center);
    vec3 result = VEC3(v3Dot(axisX, p), v3Dot(axisY, p), v3Dot(axisZ, p));
    return result;
}*/

//////////
// Frustum

/*GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(frustum) frustomFromViewProj(GROUNDED_MATH_PREFIX(mat4) viewProj) {
    GROUNDED_MATH_PREFIX(frustum) result = {{
        //TODO: Maybe should be columns instead of rows
        v4Add(viewProj.rows[3], viewProj.rows[0]),
        v4Subtract(viewProj.rows[3], viewProj.rows[0]),
        v4Add(viewProj.rows[3], viewProj.rows[1]),
        v4Subtract(viewProj.rows[3], viewProj.rows[1]),
        v4Subtract(viewProj.rows[3], viewProj.rows[2]),
        viewProj.rows[2], // maybe also v4Add(viewProj.rows[3], viewProj.rows[2]),
    }};
}*/

/*GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec3) unprojectWithFrustum(GROUNDED_MATH_PREFIX(mat4) model, GROUNDED_MATH_PREFIX(frustum) projectionFrustum, GROUNDED_MATH_PREFIX(vec3) v) {
    vec3 point = {
        .x = (v.x + projectionFrustum.frustumPlanes[0]) * -v.z * projectionFrustum.frustumPlanes[1],
        .y = (v.y + projectionFrustum.frustumPlanes[2]) * -v.z * projectionFrustum.frustumPlanes[1],
    };
    vec3 center = matMultiplyVec3(model, VEC3(0.0f, 0.0f, 0.0f));
    vec3 axisX = matMultiplyVec3(model, VEC3(1.0f, 0.0f, 0.0f));
    vec3 axisY = matMultiplyVec3(model, VEC3(0.0f, 1.0f, 0.0f));
    vec3 axisZ = matMultiplyVec3(model, VEC3(0.0f, 0.0f, 1.0f));
}*/

////////////////
// Other helpers
GROUNDED_FUNCTION_INLINE float degToRad(float angleInDeg) {
    float result = angleInDeg * 0.01745329f;
    return result;
}

GROUNDED_FUNCTION_INLINE float radToDeg(float angleInRad) {
    float result = angleInRad * 57.29578f;
    return result;
}

GROUNDED_FUNCTION_INLINE bool v2InRect(GROUNDED_MATH_PREFIX(vec2) min, GROUNDED_MATH_PREFIX(vec2) max, GROUNDED_MATH_PREFIX(vec2) point) {
    bool result = (min.x <= point.x && point.x < max.x && min.y <= point.y && point.y < max.y);
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec2) v2QuadraticBezierPoint(GROUNDED_MATH_PREFIX(vec2) p0, GROUNDED_MATH_PREFIX(vec2) p1, GROUNDED_MATH_PREFIX(vec2) p2, float t) {
    float u = 1.0 - t;
    GROUNDED_MATH_PREFIX(vec2) result = {
        .x = u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x,
        .y = u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) u32ARGBToColor(u32 color) {
    GROUNDED_MATH_PREFIX(vec4) result = {
        .r =  ((color >> 16) & 0xFF) / 255.0f,
        .g =  ((color >> 8) & 0xFF) / 255.0f,
        .b =  ((color >> 0) & 0xFF) / 255.0f,
        .a =  ((color >> 24) & 0xFF) / 255.0f,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE u32 colorToU32ARGB(GROUNDED_MATH_PREFIX(vec4) color) {
    u32 r = (u32)(CLAMP(0.0f, color.r, 1.0f) * 255.0f);
    u32 g = (u32)(CLAMP(0.0f, color.g, 1.0f) * 255.0f);
    u32 b = (u32)(CLAMP(0.0f, color.b, 1.0f) * 255.0f);
    u32 a = (u32)(CLAMP(0.0f, color.a, 1.0f) * 255.0f);
    u32 result = (a << 24) | (r << 16) | (g << 8) | b;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) u32RGBAToColor(u32 color) {
    GROUNDED_MATH_PREFIX(vec4) result = {
        .r =  ((color >> 24) & 0xFF) / 255.0f,
        .g =  ((color >> 16) & 0xFF) / 255.0f,
        .b =  ((color >> 8) & 0xFF) / 255.0f,
        .a =  ((color >> 0) & 0xFF) / 255.0f,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE u32 colorToU32RGBA(GROUNDED_MATH_PREFIX(vec4) color) {
    u32 r = (u32)(CLAMP(0.0f, color.r, 1.0f) * 255.0f);
    u32 g = (u32)(CLAMP(0.0f, color.g, 1.0f) * 255.0f);
    u32 b = (u32)(CLAMP(0.0f, color.b, 1.0f) * 255.0f);
    u32 a = (u32)(CLAMP(0.0f, color.a, 1.0f) * 255.0f);
    u32 result = (r << 24) | (g << 16) | (b << 8) | (a << 0);
    return result;
}

GROUNDED_FUNCTION_INLINE float srgbScalarToLinear(float srgb) {
    float result = (srgb <= 0.04045f) ? srgb / 12.92f : powf((srgb + 0.055f) / 1.055f, 2.4f);
    return result; 
}

GROUNDED_FUNCTION_INLINE float linearScalarToSrgb(float linear) {
    float result = (linear <= 0.0031308f) ? linear * 12.92f : 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) srgbColorToLinear(GROUNDED_MATH_PREFIX(vec4) srgb) {
    GROUNDED_MATH_PREFIX(vec4) result = {
        .r = srgbScalarToLinear(srgb.r),
        .g = srgbScalarToLinear(srgb.g),
        .b = srgbScalarToLinear(srgb.b),
        .a = srgb.a,
    };
    return result;
}

GROUNDED_FUNCTION_INLINE GROUNDED_MATH_PREFIX(vec4) linearColorToSrgb(GROUNDED_MATH_PREFIX(vec4) linear) {
    GROUNDED_MATH_PREFIX(vec4) result = {
        .r = linearScalarToSrgb(linear.r),
        .g = linearScalarToSrgb(linear.g),
        .b = linearScalarToSrgb(linear.b),
        .a = linear.a
    };
    return result;
}

GROUNDED_FUNCTION_INLINE u64 nextPowerOf2(u64 value) {
    if(value == 0) return 1;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    value++;
    return value;
}

GROUNDED_FUNCTION_INLINE u64 prevPowerOf2(u64 value) {
    if(value == 0) return 0;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value - (value >> 1);
}

#endif // GROUNDED_MATH_H
