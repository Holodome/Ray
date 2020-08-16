#include "ray_math.h"

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
asin32(f32 a)
{
    f32 result = __builtin_asinf(a);
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


inline Vec2
vec2_neg(Vec2 a)
{
    Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

inline Vec2
vec2_add(Vec2 a, Vec2 b)
{
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline Vec2 
vec2_sub(Vec2 a, Vec2 b)
{
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline Vec2
vec2_div(Vec2 a, Vec2 b)
{
    Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

inline Vec2
vec2_mul(Vec2 a, Vec2 b)
{
    Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

inline Vec2
vec2_divs(Vec2 a, f32 b)
{
    Vec2 result;
    result.x = a.x / b;
    result.y = a.y / b;
    return result;
}

inline Vec2
vec2_muls(Vec2 a, f32 b)
{
    Vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

inline Vec2
vec2(f32 x, f32 y)
{
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline Vec2
vec2s(f32 s)
{
    Vec2 result;
    result.x = s;
    result.y = s;
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
cross(Vec3 a, Vec3 b)
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

#if 1 

inline Vec3 
vec3_normalize(Vec3 a)
{
    f32 coef    = reciprocal32(vec3_length(a));
    Vec3 result = vec3_muls(a, coef);
    return result;
}

#else 

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

#endif 

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



inline Mat4x4 
mat4x4(void)
{
	Mat4x4 result = { 0 };
	return result;
}

inline Mat4x4 
mat4x4d(f32 d)
{
	Mat4x4 result = 
    {{
			{d, 0, 0, 0},
			{0, d, 0, 0},
			{0, 0, d, 0},
			{0, 0, 0, d},
		}};
	return result;
}


inline Mat4x4 
mat4x4_identity(void)
{
	return mat4x4d(1.0f);
}

inline Mat4x4
mat4x4_translate(Vec3 t) {
	Mat4x4 result = mat4x4_identity();
    result.v[0].xyz = t;
	return result;
}

inline Mat4x4
mat4x4_scale(Vec3 s) {
	Mat4x4 result =
    {{
			{s.x,   0,   0,  0},
			{0,   s.y,   0,  0},
			{0,     0, s.z,  0},
			{0,     0,   0,  1},
		}};
	return result;
}

inline Mat4x4
mat4x4_rotation_x(f32 angle) {
	const f32 c = cos32(angle);
	const f32 s = sin32(angle);
	Mat4x4 r =
	{{
			{1, 0, 0, 0},
			{0, c,-s, 0},
			{0, s, c, 0},
			{0, 0, 0, 1}
		}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_y(f32 angle) {
	const f32 c = cos32(angle);
	const f32 s = sin32(angle);
	Mat4x4 r =
	{{
			{ c, 0, s, 0},
			{ 0, 1, 0, 0},
			{-s, 0, c, 0},
			{ 0, 0, 0, 1}
		}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_z(f32 angle) {
	const f32 c = cos32(angle);
	const f32 s = sin32(angle);
	Mat4x4 r =
	{{
			{c,-s, 0, 0},
			{s, c, 0, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 1}
		}};
	return(r);
}

inline Mat4x4
mat4x4_rotation(f32 angle, Vec3 a) {
	const f32 c = cos32(angle);
	const f32 s = sin32(angle);
	a = vec3_normalize(a);
	
	const f32 tx = (1.0f - c) * a.x;
	const f32 ty = (1.0f - c) * a.y;
	const f32 tz = (1.0f - c) * a.z;
	
	Mat4x4 r =
	{{
			{c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.z, 0},
			{    ty * a.x,		     ty * a.y + c,       ty * a.z + s * a.x, 0},
			{    tz * a.x + s * a.x, tz * a.y - s * a.x, tz * a.z + c,       0},
			{0, 0, 0, 1}
		}};
	return(r);
}

inline Mat4x4 
mat4x4_orthographic3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)
{
    Mat4x4 result =
	{{
			{2.0f / (r - l),    0,                   0,                 0},
			{0,                 2.0f / (t - b),      0,                 0},
			{0,                 0,                  -2.0f / (f - n),    0},
			{-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1}
		}};
	return result;
}

inline Mat4x4 
mat4x4_orthographic2d(f32 l, f32 r, f32 b, f32 t)
{
	Mat4x4 result =
	{{
			{2.0f / (r - l),    0,                   0, 0},
			{0,                 2.0f / (t - b),      0, 0},
			{0,                 0,                  -1, 0},
			{-(r + l) / (r - l), -(t + b) / (t - b), 0, 1}
		}};
	return result;
}

inline Mat4x4 
mat4x4_look_at(Vec3 pos, Vec3 target)
{
    Vec3 world_up = {{ 0, 0, 1 }};
    
    Vec3 camera_z = vec3_normalize(vec3_sub(target, pos));
    Vec3 camera_x = vec3_normalize(cross(world_up, camera_z));
    Vec3 camera_y = vec3_normalize(cross(camera_z, camera_x));
    
    Mat4x4 result = 
    {{
			{camera_x.x, camera_y.x, -camera_z.x, 0}, 
			{camera_x.y, camera_y.y, -camera_y.y, 0}, 
			{camera_x.z, camera_y.y, -camera_y.z, 0}, 
			{-vec3_dot(camera_x, pos), -vec3_dot(camera_y, pos), vec3_dot(camera_z, pos), 1.0f} 
		}};
    
    return result;
}

inline Mat4x4 
mat4x4_mul(Mat4x4 a, Mat4x4 b)
{
    Mat4x4 result;
	for(u32 r = 0; 
        r < 4;
        ++r)
    {
		for(u32 c = 0;
            c < 4;
            ++c)
        {
			for(u32 i = 0;
                i < 4;
                ++i) 
            {
				result.e[r][c] += a.e[r][i] * b.e[i][c];
			}
		}
	}
	return result;
}