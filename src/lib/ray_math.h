
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

inline f32 square(f32 a) { return a * a; }
inline f32 cube(f32 a) { return a * a * a; }

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
inline f32 max32(f32 a, f32 b);
inline f32 min32(f32 a, f32 b);
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
inline f32
schlick(f32 cosine, f32 ref_idx)
{
    f32 r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    f32 result = r0 + (1.0f - r0) * pow32((1.0f - cosine), 5.0f);
    return result;
}

//
// Vector2 
//


typedef union {
    struct {
        f32 x;
        f32 y;
    };
    struct {
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


//
// Vector3
//


typedef union {
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
	Vec2 xy;
	struct { 
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

inline f32  dot(Vec3 a, Vec3 b);
inline Vec3 cross(Vec3 a, Vec3 b);
inline f32  vec3_length_sq(Vec3 a);
inline f32  vec3_length(Vec3 a);
inline Vec3 normalize(Vec3 a);
inline Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t);

inline Vec3 vec3_from_vec2(Vec2 xy, f32 z) { return (Vec3){ .x = xy.x, .y = xy.y, .z = z }; }
inline Vec3 vec3_reflect(Vec3 v, Vec3 normal) { return vec3_sub(v, vec3_muls(normal, 2.0f * dot(v, normal))); }

inline Vec2 
unit_sphere_get_uv(Vec3 p)
{
    f32 phi   = atan232(p.y, p.x);
    f32 theta = asin32(p.z);
    
    Vec2 result = {   
        .u = 1.0f - (phi + PI32) / TWO_PI32,
        .v = (theta + HALF_PI32) / PI32
    };
    return result;    
}



//
// Vector4
//


typedef union {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    struct {
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


//
// Color converters
//


// http://entropymine.com/imageworsener/srgbformula/
// Converts from linear color value to SRGB normalized in 0-1 space
inline f32  linear1_to_srgb1(f32 l);
// Convertes from color values from linear to SRGB normalized in 0-255 space 
inline Vec4 linear1_to_srgb255(Vec4 a);
// Packs 4 float values (normalized in 0-255) in single 32-bit value where each color component is single byte
inline u32 rgba_pack_4x8(Vec4 a);


//
// 4x4 Matrix 
//


typedef union {
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
inline Mat4x4 mat4x4_perspective(f32 fovy, f32 aspect, f32 near, f32 far);
inline Mat4x4 mat4x4_look_at(Vec3 pos, Vec3 target);
inline Mat4x4 mat4x4_mul(Mat4x4 a, Mat4x4 b);

//
// Rect2
//


typedef struct {
	Vec2 min;
	Vec2 max;
} Rect2;

// Constructors for rects. Functions ending with v take vectors as parameters
inline Rect2 rect2(f32 x0, f32 y0, f32 x1, f32 y1);
inline Rect2 rect2v(Vec2 min, Vec2 max);
inline Rect2 rect2_point_size(f32 x, f32 y, f32 w, f32 h);
inline Rect2 rect2_point_sizev(Vec2 point, Vec2 size);
// Corners of rect
inline Vec2 rect2_top_left(Rect2 r);
inline Vec2 rect2_top_right(Rect2 r);
inline Vec2 rect2_bottom_left(Rect2 r);
inline Vec2 rect2_bottom_right(Rect2 r);
// Different other rect points
inline Vec2 rect2_center(Rect2 r);
inline Vec2 rect2_size(Rect2 r);
inline f32 rect2_width(Rect2 r);
inline f32 rect2_height(Rect2 r);
inline f32 rect2_center_x(Rect2 r);
inline f32 rect2_center_y(Rect2 r);
// Writes all rect corners in given array
inline void rect2_store_pointsa(Rect2 rect, Vec2 points[4]);
// Wrties all rect coreners in corresponding parameters
inline void rect2_store_points(Rect2 rect,
                   Vec2 top_left[static 1], Vec2 bottom_left[static 1],
				   Vec2 top_right[static 1], Vec2 bottom_right[static 1]);
// Checks if point is inside of the rect
inline bool rect2_collide_point(Rect2 rect, Vec2 point);
// Checks if two rects intersect
inline bool rect2_collide(Rect2 a, Rect2 b);
// Moves rect by adding dist to both min and max
inline Rect2 rect2_move(Rect2 rect, Vec2 dist);
// Clips one rect in other - child is cut to fully be inside of parent
inline Rect2 rect2_clip(Rect2 parent, Rect2 rect);

typedef struct {
    Vec3 min;
    Vec3 max;
} Box3;

Box3 
box3_join(Box3 a, Box3 b)
{
    Box3 result;
    
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
