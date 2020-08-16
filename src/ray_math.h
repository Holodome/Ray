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
	Vec2 xy;
	struct 
	{ 
		f32 __x;
		Vec2 yz;
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
inline Vec3 cross(Vec3 a, Vec3 b);
inline f32  vec3_length_sq(Vec3 a);
inline f32  vec3_length(Vec3 a);
inline Vec3 vec3_normalize(Vec3 a);
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
    f32 phi   = atan232(p.y, p.x);
    f32 theta = asin32(p.z);
    
    result.u = 1.0f - (phi + PI32) / TWO_PI32;
    result.v = (theta + HALF_PI32) / PI32;
    
    return result;    
}

typedef struct 
{
	Vec2 min;
	Vec2 max;
} Rect2;


inline Rect2
rect2v(Vec2 min, Vec2 max)
{
    Rect2 result;
    result.min = min;
    result.max = max;
    return result;
}

inline Rect2
rect2(f32 x0, f32 y0, f32 x1, f32 y1)
{
    Rect2 result = rect2v(vec2(x0, y0), vec2(x1, y1));
    return result;
}

inline Rect2
rect2_point_sizev(Vec2 point, Vec2 size)
{
    Rect2 result = rect2v(point, vec2_add(point, size));
    return result;
}

inline Rect2
rect2_point_size(f32 x, f32 y, f32 w, f32 h)
{
    Rect2 result = rect2_point_sizev(vec2(x, y), vec2(w, h));
    return result;
}

inline Vec2
rect2_top_left(Rect2 r)
{
    return r.min;
}

inline Vec2
rect2_top_right(Rect2 r)
{
    return vec2(r.max.x, r.min.y);
}

inline Vec2
rect2_bottom_left(Rect2 r)
{
    return vec2(r.min.x, r.max.y);
}

inline Vec2
rect2_bottom_right(Rect2 r)
{
    return r.max;
}

inline Vec2
rect2_center(Rect2 r)
{
    return vec2_add(r.min, vec2_muls(vec2_sub(r.max, r.min), 0.5f));
}

inline Vec2
rect2_size(Rect2 r)
{
    return vec2_sub(r.max, r.min);
}

inline f32
rect2_width(Rect2 r)
{
    return r.max.x - r.min.x;
}

inline f32
rect2_height(Rect2 r)
{
    return r.max.y - r.min.y;
}

inline f32
rect2_center_x(Rect2 r)
{
    return r.min.x + (r.max.x - r.min.x) * 0.5f;
}

inline f32
rect2_center_y(Rect2 r)
{
    return r.min.y + (r.max.y - r.min.y) * 0.5f;
}

inline void
rect2_store_pointsa(Rect2 rect, Vec2 points[4])
{
    points[0] = rect2_top_left(rect);
    points[1] = rect2_bottom_left(rect);
    points[2] = rect2_top_right(rect);
    points[3] = rect2_bottom_right(rect);
}

inline void
rect2_store_points(Rect2 rect,
                   Vec2 *top_left, Vec2 *bottom_left,
				   Vec2 *top_right, Vec2 *bottom_right)
{
    *top_left = rect2_top_left(rect);
    *bottom_left = rect2_bottom_left(rect);
    *top_right = rect2_top_right(rect);
    *bottom_right = rect2_bottom_right(rect);
}

inline bool
rect2_collide_point(Rect2 rect, Vec2 point)
{
	bool result = (((rect.min.x < point.x) && (point.x < rect.max.x)) &&
				   ((rect.min.y < point.y) && (point.x < rect.max.y)));
	return result;
}

inline bool
rect2_collide(Rect2 a, Rect2 b)
{
	bool result = true;
	
	if ((a.max.x < b.min.x || a.min.x > b.max.x) ||
		(a.max.y < b.min.y || a.min.y > b.min.y))
	{
		result = false;
	}
	
	return result;
	
}

inline Rect2
rect2_move(Rect2 rect, Vec2 dist)
{
	Rect2 result = rect2v(vec2_add(rect.min, dist),
						  vec2_add(rect.max, dist));
	return result;
}

inline Rect2
rect2_clip(Rect2 parent, Rect2 rect)
{
    Rect2 result;
    result.min.x = max(parent.min.x, rect.min.x);
    result.min.y = max(parent.min.y, rect.min.y);
    result.max.x = min(parent.max.x, rect.max.x);
    result.max.y = min(parent.max.y, rect.max.y);
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

inline Vec3 
vec3_rotate_z(Vec3 v, f32 sin_theta, f32 cos_theta)
{
    Vec3 result;
    result.x = cos_theta * v.x - sin_theta * v.y;
    result.y = sin_theta * v.x + cos_theta * v.y;
    return result;
}

#define MATH_H 1
#endif
