#include "RayMath.h"

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

inline f32 
pow32(f32 a, f32 b)
{
    f32 result = __builtin_powf(a, b);
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

inline f32
abs32(f32 a)
{
    u32 temp = *(u32 *)&a;
    temp &= 0x7FFFFFFF;
    f32 result = *(f32 *)&temp;
    return result;
}


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
vec3_divs(Vec3 a, f32 b)
{
    Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

inline Vec3 
vec3_muls(Vec3 a, f32 b)
{
    Vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
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
vec4_divs(Vec4 a, f32 b)
{
    Vec4 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    result.w = a.w / b;
    return result;
}

inline Vec4
vec4_muls(Vec4 a, f32 b)
{
    Vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
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

inline f32 
vec3_dot(Vec3 a, Vec3 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

inline Vec3 
vec3_cross(Vec3 a, Vec3 b)
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
    f32 result = vec3_dot(a, a);
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
vec3_normalize(Vec3 a)
{
    Vec3 result = {0};
    
    f32 length_sq = vec3_length_sq(a);
    if(length_sq > square(0.0001f))
    {
        result = vec3_mul(a, vec3s(reciprocal32(sqrt32(length_sq))));
    }
    
    return result;
}

inline Vec3
vec3_lerp(Vec3 a, Vec3 b, f32 t)
{
    Vec3 result;
    
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    
    return result;
}

inline Vec4 
linear1_to_srgb255(Vec4 a)
{
    Vec4 result = {0};
    
    f32 one255 = 255.0f;
    
    result.r = one255 * sqrt32(a.r);
    result.g = one255 * sqrt32(a.g);
    result.b = one255 * sqrt32(a.b);
    result.a = one255 * a.a;
    
    return result;
}

inline u32
rgba_pack_4x8(Vec4 a)
{
    Vec4 byte_normalized = a;
    u32  result = ((round32(byte_normalized.x) << 0) |
                   (round32(byte_normalized.y) << 8) |
                   (round32(byte_normalized.z) << 16) |
                   (round32(byte_normalized.w) << 24));
    return result;
}