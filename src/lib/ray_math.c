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
max32(f32 a, f32 b)
{
    return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
}

inline f32 
min32(f32 a, f32 b)
{
    return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
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
dot(Vec3 a, Vec3 b)
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
    f32 result = dot(a, a);
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
normalize(Vec3 a)
{
    f32 coef    = reciprocal32(vec3_length(a));
    Vec3 result = vec3_muls(a, coef);
    return result;
}

#else 

inline Vec3 
normalize(Vec3 a)
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

inline f32
linear1_to_srgb1(f32 l)
{
    if (l < 0)
        l = 0;
    if (l > 1)
        l = 1;

    f32 s = l * 12.92f;
    if (l > 0.0031308f)
    {
        s = 1.055f * pow32(l, reciprocal32(2.4f)) - 0.055f;
    }
    return s;
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
	Mat4x4 result = {0};
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
    result.v[3].xyz = t;
    result.v[3].w = 1.0f;
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
mat4x4_rotate_x(f32 angle) {
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
mat4x4_rotate_y(f32 angle) {
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
mat4x4_rotate_z(f32 angle) {
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
mat4x4_rotate(f32 angle, Vec3 a) {
	const f32 c = cos32(angle);
	const f32 s = sin32(angle);
	a = normalize(a);
	
	const f32 tx = (1.0f - c) * a.x;
	const f32 ty = (1.0f - c) * a.y;
	const f32 tz = (1.0f - c) * a.z;
	
	Mat4x4 r =
	{{
			{c + tx * a.x, 		     tx * a.y + s * a.z, tx * a.z - s * a.y, 0},
			{    ty * a.x - s * a.z, ty * a.y + c,       ty * a.z + s * a.x, 0},
			{    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0},
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
    
    Vec3 camera_z = normalize(vec3_sub(target, pos));
    Vec3 camera_x = normalize(cross(world_up, camera_z));
    Vec3 camera_y = normalize(cross(camera_z, camera_x));
    
    Mat4x4 result = 
    {{
        {camera_x.x, camera_y.x, -camera_z.x, 0}, 
        {camera_x.y, camera_y.y, -camera_y.y, 0}, 
        {camera_x.z, camera_y.y, -camera_y.z, 0}, 
        {-dot(camera_x, pos), -dot(camera_y, pos), dot(camera_z, pos), 1.0f} 
    }};
    
    return result;
}

inline Mat4x4 
mat4x4_mul(Mat4x4 a, Mat4x4 b)
{
    Mat4x4 result = {0};
	for(u32 i = 0; 
        i < 4;
        ++i)
    {
		for(u32 j = 0;
            j < 4;
            ++j)
        {
            result.e[i][j] = a.e[0][j] * b.e[i][0] +
                             a.e[1][j] * b.e[i][1] +
                             a.e[2][j] * b.e[i][2] +
                             a.e[3][j] * b.e[i][3];
		}
	}
	return result;
}

inline Mat4x4 
mat4x4_perspective(f32 fovy, f32 aspect, f32 near, f32 far)
{
    Mat4x4 result = {0};
    
    f32 thf = tan32(fovy / 2.0f);

	result.e[0][0] = 1.0f / (aspect * thf);
	result.e[1][1] = 1.0f / (thf);
	result.e[2][2] = -(far + near) / (far - near);
	result.e[2][3] = -1.0f;
	result.e[3][2] = -2.0f* far * near / (far - near);
    
    return result;
}

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
				   ((rect.min.y < point.y) && (point.y < rect.max.y)));
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