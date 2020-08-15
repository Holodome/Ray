// @NOTE(hl): Put this outside of include guards
#ifdef max
#undef max 
#endif 

#ifdef min
#undef min
#endif 

#define max(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a > _b) ? (_a) : (_b); })
#define min(a, b) ({__typeof__(a) _a = a; __typeof__(b) _b = b; (_a < _b) ? (_a) : (_b); })
#define swap(a, b) ({__typeof__(a) temp = a; a = b; b = temp; (void)0; })

#if !defined(MATH_H)

#include "common.h"

// SSE extensions. We assume everyone has them. 
// https://store.steampowered.com/hwsurvey/Steam-Hardware-Software-Survey-Welcome-to-Steam?platform=pc 
// > Steam says that 100% of its users have SSE
#include <xmmintrin.h>

#define HALF_PI32 1.57079632679f
#define PI32      3.14159265359f
#define TWO_PI32  6.28318530718f

#define bytes(x)     ((u64)x)
#define kilobytes(x) (bytes(x) << 10)
#define megabytes(x) (kilobytes(x) << 10)

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
inline f32 asin32(f32 a);
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
    };
    struct 
    {
        f32 u;
        f32 v;
    };
    f32 e[2];
} Vec2;


// @NOTE(hl): Define math operators needed to use vectors
// In C++ we would use operator overloading, but C does not support it, 
// so we have to go with operator-like functions
//
// Lower-case vector name function is 'constructor', that initializes vector from given components
// Lower-case vector name ending with s constructs vector from single scalar
// add, sub, div, mul correspond to addition, subtraction, division, multiplication. Multiplication is component-wise (hadamard product), not inner or cross product

inline Vec2 vec2(f32 x, f32 y);
inline Vec2 vec2s(f32 s);
inline Vec2 vec2_neg(Vec2 a);
inline Vec2 vec2_add(Vec2 a, Vec2 b);
inline Vec2 vec2_sub(Vec2 a, Vec2 b);
inline Vec2 vec2_div(Vec2 a, Vec2 b);
inline Vec2 vec2_mul(Vec2 a, Vec2 b);
inline Vec2 vec2_divs(Vec2 a, f32 b);
inline Vec2 vec2_muls(Vec2 a, f32 b);

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

inline Vec4 vec4(f32 x, f32 y, f32 z, f32 w);
inline Vec4 vec4s(f32 s);
inline Vec4 vec4_neg(Vec4 a);
inline Vec4 vec4_add(Vec4 a, Vec4 b);
inline Vec4 vec4_sub(Vec4 a, Vec4 b);
inline Vec4 vec4_div(Vec4 a, Vec4 b);
inline Vec4 vec4_mul(Vec4 a, Vec4 b);
inline Vec4 vec4_divs(Vec4 a, f32 b);
inline Vec4 vec4_muls(Vec4 a, f32 b);

typedef union  
{
	f32  e[4][4];
    Vec4 v[4];
} Mat4x4;

inline Mat4x4 mat4x4(void);
inline Mat4x4 mat4x4d(f32 d);
inline Mat4x4 mat4x4_identity(void);
inline Mat4x4 mat4x4_translate(Vec3 v);
inline Mat4x4 mat4x4_scale(Vec3 v);
inline Mat4x4 mat4x4_rotate_x(f32 angle);
inline Mat4x4 mat4x4_rotate_y(f32 angle);
inline Mat4x4 mat4x4_rotate_z(f32 angle);
inline Mat4x4 mat4x4_rotate(f32 angle, Vec3 v);
inline Mat4x4 mat4x4_orthographic3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
inline Mat4x4 mat4x4_orthographic2d(f32 l, f32 r, f32 b, f32 t);
inline Mat4x4 mat4x4_look_at(Vec3 pos, Vec3 target);
inline Mat4x4 mat4x4_mul(Mat4x4 a, Mat4x4 b);

// @TODO(hl): Refactor this somehow
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

// https://en.wikipedia.org/wiki/Schlick%27s_approximation
inline f32
schlick(f32 cosine, f32 ref_idx)
{
    f32 r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    f32 result = r0 + (1.0f - r0) * pow32((1.0f - cosine), 5.0f);
    return result;
}

Vec2 
unit_sphere_get_uv(Vec3 p)
{
    Vec2 result;
    f32 phi   = atan232(p.z, p.x);
    f32 theta = asin32(p.y);
    
    result.u = 1.0f - (phi + PI32) / TWO_PI32;
    result.v = (theta + HALF_PI32) / PI32;
    
    return result;    
}

typedef struct 
{
    Vec3 min;
    Vec3 max;
} AABB;

AABB 
aabb_join(AABB a, AABB b)
{
    AABB result;
    
    result.min.x = min(a.min.x, b.min.x);
    result.min.x = min(a.min.y, b.min.y);
    result.min.x = min(a.min.z, b.min.z);
    
    result.max.x = max(a.max.x, b.max.x);
    result.max.x = max(a.max.y, b.max.y);
    result.max.x = max(a.max.z, b.max.z);
    
    return result;    
}

#define MATH_H 1
#endif
