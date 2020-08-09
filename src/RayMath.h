#if !defined(MATH_H)

#include "Common.h"

// SSE extensions. We assume everyone has them. 
// https://store.steampowered.com/hwsurvey/Steam-Hardware-Software-Survey-Welcome-to-Steam?platform=pc 
// > Steam says that 100% of its users have SSE
#include <xmmintrin.h>

#define bytes(x)     ((u64)x)
#define kilobytes(x) (bytes(x) << 10)
#define megabytes(x) (kilobytes(x) << 10)

// @NOTE(hl): Relies on clang/gcc __typeof__ extension.
#define maximum(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a > _b) ? (_a) : (_b); })
#define minimum(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a < _b) ? (_a) : (_b); })

#define square(x) ({__typeof__(x) _x = x; _x * _x; })
#define cube(x)   ({__typeof__(x) _x = x; _x * _x * _x; })


// General-purpose functions

// Functions end with 32 to differ from math.h ones. Also could make them capitalized, but it seems to break code style
// @NOTE(hl): Calculates fast 1.0f / x using special instruction, which is way faster than actually dividing
inline f32
reciprocal32(f32 a)
{
    f32 result = _mm_cvtss_f32(_mm_rcp_ss(_mm_set_ss(a)));
    return result;
}

// @NOTE(hl): Performs truncation, like (i32)2.5f
inline i32
truncate32(f32 a)
{
    i32 result = _mm_cvtt_ss2si(_mm_set_ss(a));
    return result;
}

inline i32
round32(f32 a)
{
    i32 result = _mm_cvt_ss2si(_mm_set_ss(a + a + 0.5f)) >> 1;
    return result;
}

inline i32
floor32(f32 a)
{
    i32 result = _mm_cvt_ss2si(_mm_set_ss(a + a - 0.5f)) >> 1;
    return result;
}

inline i32
ceil32(f32 a)
{
    i32 result = -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (a + a))) >> 1);
    return result;
}

inline f32
sqrt32(f32 a)
{
    f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a)));
    return result;
}

inline f32
rsqrt32(f32 a)
{
    f32 result = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(a)));
    return result;
}

inline f32
sin32(f32 a)
{
    f32 result = __builtin_sinf(a);
    return result;
}

inline f32
cos32(f32 a)
{
    f32 result = __builtin_cosf(a);
    return result;
}

inline f32
atan232(f32 y, f32 x)
{
    f32 result = __builtin_atan2f(y, x);
    return result;
}

inline f32
tan32(f32 a)
{
    f32 result = __builtin_tanf(a);
    return result;
}

inline f32
mod32(f32 a, f32 b)
{
	f32 result = __builtin_fmodf(a, b);
	return result;
}

inline bool
is_power_of_two(u32 x)
{
	bool result = (x & (x - 1)) == 0;
	return result;
}

inline f32
lerp(f32 a, f32 b, f32 t)
{
	f32 result = a + (b - a) * t;
	return result;
}

inline f32
clamp(f32 a, f32 low, f32 high)
{
	f32 result = ((a < low) ? low : (a > high) ? high : a);
	return result;
}

inline f32
clamp01(f32 a)
{
	f32 result = clamp(a, 0, 1);
	return result;
}

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


inline Vec3 
vec3_neg(Vec3 a)
{
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

inline Vec3 
vec3_add(Vec3 a, Vec3 b)
{
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline Vec3 
vec3_sub(Vec3 a, Vec3 b)
{
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline Vec3 
vec3_div(Vec3 a, Vec3 b)
{
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

inline Vec3 
vec3_mul(Vec3 a, Vec3 b)
{
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

inline Vec3 
vec3(f32 x, f32 y, f32 z)
{
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline Vec3 
vec3s(f32 s)
{
    Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

inline Vec4 
vec4_neg(Vec4 a)
{
    Vec4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}

inline Vec4 
vec4_add(Vec4 a, Vec4 b)
{
    Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline Vec4 
vec4_sub(Vec4 a, Vec4 b)
{
    Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

inline Vec4 
vec4_div(Vec4 a, Vec4 b)
{
    Vec4 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

inline Vec4 
vec4_mul(Vec4 a, Vec4 b)
{
    Vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

inline Vec4 
vec4(f32 x, f32 y, f32 z, f32 w)
{
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

inline Vec4 
vec4s(f32 s)
{
    Vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

inline u32
vec4_normalized_to_u32(Vec4 v)
{
    Vec4 byte_normalized = vec4_mul(v, vec4s(0xFF));
    u32  result = ((round32(byte_normalized.x) << 0) |
                   (round32(byte_normalized.y) << 8) |
                   (round32(byte_normalized.z) << 16) |
                   (round32(byte_normalized.w) << 24));
    return result;
}

inline f32 
vec3_dot_product(Vec3 a, Vec3 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

inline Vec3 
vec3_cross_product(Vec3 a, Vec3 b)
{
    Vec3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

inline f32 
vec3_length_sq(Vec3 a)
{
    f32 result = vec3_dot_product(a, a);
    return result;
}

inline f32 
vec3_length(Vec3 a)
{
    f32 result = sqrt32(vec3_length_sq(a));
    return result;
}

inline Vec3 
vec3_normalize_fast(Vec3 a)
{
    f32 coef    = reciprocal32(vec3_length(a));
    Vec3 result = vec3_mul(a, vec3s(coef));
    return result;
}

inline Vec3 
vec3_normalize_slow(Vec3 a)
{
    Vec3 result = {0};
    
    f32 length_sq = vec3_length_sq(a);
    if(length_sq > square(0.0001f))
    {
        result = vec3_mul(a, vec3s(reciprocal32(sqrt32(length_sq))));
    }
    
    return result;
}

#define MATH_H 1
#endif
