#if !defined(MATH_H)

#include "Common.h"

// SSE extensions. We assume everyone has them. 
// https://store.steampowered.com/hwsurvey/Steam-Hardware-Software-Survey-Welcome-to-Steam?platform=pc 
// > Steam says that 100% of its users have SSE
#include <xmmintrin.h>

#define bytes(x)     ((u64)x)
#define kilobytes(x) (bytes(x) << 10)
#define megabytes(x) (kilobytes(x) << 10)

#define maximum(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a > _b) ? (_a) : (_b); })
#define minimum(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a < _b) ? (_a) : (_b); })

#define square(x) ({__typeof__(x) _x = x; _x * _x; })
#define cube(x)   ({__typeof__(x) _x = x; _x * _x * _x; })

// @NOTE(hl): Calculates fast 1.0f / x using special instruction, which is way faster than actually dividing
inline f32 reciprocal32(f32 a);
// @NOTE(hl): Performs truncation, like (i32)2.5f
inline i32 truncate32(f32 a);
inline i32 round32(f32 a);
inline i32 floor32(f32 a);
inline i32 ceil32(f32 a);
inline f32 sqrt32(f32 a);
inline f32 rsqrt32(f32 a);
inline f32 sin32(f32 a);
inline f32 cos32(f32 a);
inline f32 atan232(f32 y, f32 x);
inline f32 tan32(f32 a);
inline f32 mod32(f32 a, f32 b);
inline f32 pow32(f32 a, f32 b);
inline bool is_power_of_two(u32 x);
inline f32 lerp(f32 a, f32 b, f32 t);
inline f32 clamp(f32 a, f32 low, f32 high);
inline f32 clamp01(f32 a);
inline f32 abs32(f32 a);

typedef union 
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
    };
    struct
    {
        f32 r;
        f32 g;
        f32 b;
    };
    f32 e[3];
} Vec3;

typedef union 
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    Vec3 xyz;
    Vec3 rgb;
    f32 e[4];
} Vec4;

// @NOTE(hl): Define math operators needed to use vectors
// In C++ we would use operator overloading, but C does not support it, 
// so we have to go with operator-like functions
//
// Lower-case vector name function is 'constructor', that initializes vector from given components
// Lower-case vector name ending with s constructs vector from single scalar
// add, sub, div, mul correspond to addition, subtraction, division, multiplication. Multiplication is component-wise (hadamard product), not inner or cross product
inline Vec3 vec3(f32 x, f32 y, f32 z);
inline Vec3 vec3s(f32 s);
inline Vec3 vec3_neg(Vec3 a);
inline Vec3 vec3_add(Vec3 a, Vec3 b);
inline Vec3 vec3_sub(Vec3 a, Vec3 b);
inline Vec3 vec3_div(Vec3 a, Vec3 b);
inline Vec3 vec3_mul(Vec3 a, Vec3 b);
inline Vec3 vec3_divs(Vec3 a, f32 b);
inline Vec3 vec3_muls(Vec3 a, f32 b);

inline f32  vec3_dot(Vec3 a, Vec3 b);
inline Vec3 vec3_cross(Vec3 a, Vec3 b);
inline f32  vec3_length_sq(Vec3 a);
inline f32  vec3_length(Vec3 a);
inline Vec3 vec3_normalize(Vec3 a);
inline Vec3 vec3_normalize_fast(Vec3 a);
inline Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t);


inline Vec4 vec4(f32 x, f32 y, f32 z, f32 w);
inline Vec4 vec4s(f32 s);
inline Vec4 vec4_neg(Vec4 a);
inline Vec4 vec4_add(Vec4 a, Vec4 b);
inline Vec4 vec4_sub(Vec4 a, Vec4 b);
inline Vec4 vec4_div(Vec4 a, Vec4 b);
inline Vec4 vec4_mul(Vec4 a, Vec4 b);
inline Vec4 vec4_divs(Vec4 a, f32 b);
inline Vec4 vec4_muls(Vec4 a, f32 b);

inline u32 vec4_normalized_to_u32(Vec4 v);

inline Vec4 linear1_to_srgb255(Vec4 a);

inline u32 rgba_pack_4x8(Vec4 a);

// Reflects given direction vector from given normal
inline Vec3 
vec3_reflect(Vec3 v, Vec3 normal)
{
    Vec3 result = vec3_sub(v, vec3_muls(normal, 2.0f * vec3_dot(v, normal)));
    return result;
}

#define MATH_H 1
#endif
