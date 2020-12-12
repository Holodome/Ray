#if !defined(RAY_MATH_H)

#include "general.h"

#include "ray_math_intrinsics.h"

#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define HALF_PI 1.57079632679f

#define DEG2RAD(_deg) ((f32)(_deg) * PI / 180.0f)

inline f32 
lerp(f32 a, f32 b, f32 t) {
    return (1.0f - t) * a + t * b;
}

inline f32 
clamp(f32 x, f32 low, f32 high) {
#if 1
    return max32(min32(high, x), low);
#else 
    if (x < low) {
        x = low;
    }
    if (x > high) {
        x = high;
    }
    return x;
#endif 
}

typedef union {
    struct {
        f32 x, y;
    };
    f32 e[2];
} Vec2;

inline Vec2 
v2(f32 x, f32 y) {
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

inline Vec2 
v2s(f32 s) {
    Vec2 result;
    result.x = s;
    result.y = s;
    return result;
}

inline Vec2 
v2neg(Vec2 a) {
    Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

inline Vec2 
v2add(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

inline Vec2
v2add3(Vec2 a, Vec2 b, Vec2 c) {
    return v2add(a, v2add(b, c));
}

inline Vec2 
v2sub(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

inline Vec2 
v2div(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

inline Vec2 
v2mul(Vec2 a, Vec2 b) {
    Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

inline Vec2 
v2divs(Vec2 a, f32 b) {
    Vec2 result;
    result.x = a.x / b;
    result.y = a.y / b;
    return result;
}

inline Vec2 
v2muls(Vec2 a, f32 b) {
    Vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

typedef union {
    struct {
        f32 x, y, z;  
    };
    struct {
        f32 r, g, b;  
    };
    f32 e[3];
} Vec3;

inline Vec3 
v3(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline Vec3 
v3s(f32 s) {
    Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

inline Vec3 
v3neg(Vec3 a) {
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

inline Vec3 
v3add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline Vec3
v3add3(Vec3 a, Vec3 b, Vec3 c) {
    return v3add(a, v3add(b, c));
}

inline Vec3 
v3sub(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline Vec3 
v3div(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

inline Vec3 
v3mul(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

inline Vec3 
v3divs(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

inline Vec3 
v3muls(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

inline f32 
dot(Vec3 a, Vec3 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

inline Vec3 
cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

inline f32 
length_sq(Vec3 a) {
    f32 result = dot(a, a);
    return result;
}

inline f32 
length(Vec3 a) {
    f32 result = sqrt32(length_sq(a));
    return result;
}

inline Vec3 
normalize(Vec3 a)
{
    Vec3 result = v3muls(a, rsqrt32(length_sq(a)));
    return result;
}

inline Vec3 
v3lerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

inline bool 
vec3_is_near_zero(Vec3 a) {
    const f32 epsilon = 1e-4f;
    bool result = (abs32(a.x) < epsilon) && (abs32(a.y) < epsilon) && (abs32(a.z) < epsilon);
    return result;
}

typedef union {
    struct {
        f32 x, y, z, w;
    };
    f32 e[4];
} Vec4;

inline Vec4 
v4neg(Vec4 a) {
    Vec4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}

inline Vec4 
v4add(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

inline Vec4 
v4sub(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

inline Vec4 
v4div(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

inline Vec4 
v4mul(Vec4 a, Vec4 b) {
    Vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

inline Vec4
v4divs(Vec4 a, f32 b) {
    Vec4 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    result.w = a.w / b;
    return result;
}

inline Vec4
v4muls(Vec4 a, f32 b) {
    Vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
    return result;
}

inline Vec4 
v4(f32 x, f32 y, f32 z, f32 w) {
    Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

inline Vec4 
v4s(f32 s) {
    Vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

inline f32 
v4dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline Vec4
v4normalize(Vec4 a) {
    Vec4 result = v4muls(a, rsqrt32(v4dot(a, a)));
    return result;
} 

typedef union {
    f32  e[4][4];
    struct {
        f32 m00; f32 m01; f32 m02; f32 m03;
        f32 m10; f32 m11; f32 m12; f32 m13;
        f32 m20; f32 m21; f32 m22; f32 m23;
        f32 m30; f32 m31; f32 m32; f32 m33;
    };
    f32 i[16];
} Mat4x4;

const Mat4x4 MAT4X4_IDENTITIY = { 
    .m00 = 1,
    .m11 = 1,
    .m22 = 1,
    .m33 = 1
};

inline Mat4x4
mat4x4_translate(Vec3 t) {
	Mat4x4 result = MAT4X4_IDENTITIY;
    result.e[3][0] = t.x;
    result.e[3][1] = t.y;
    result.e[3][2] = t.z;
	return result;
}

inline Mat4x4
mat4x4_scale(Vec3 s) {
	Mat4x4 result = {{
        {s.x,   0,   0,  0},
        {0,   s.y,   0,  0},
        {0,     0, s.z,  0},
        {0,     0,   0,  1},
    }};
	return result;
}

inline Mat4x4
mat4x4_rotation_x(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{1, 0, 0, 0},
		{0, c,-s, 0},
		{0, s, c, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_y(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{ c, 0, s, 0},
		{ 0, 1, 0, 0},
		{-s, 0, c, 0},
		{ 0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation_z(f32 angle) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	Mat4x4 r = {{
		{c,-s, 0, 0},
		{s, c, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_rotation(f32 angle, Vec3 a) {
	const f32 c = cosf(angle);
	const f32 s = sinf(angle);
	a = normalize(a);

	const f32 tx = (1.0f - c) * a.x;
	const f32 ty = (1.0f - c) * a.y;
	const f32 tz = (1.0f - c) * a.z;

	Mat4x4 r = {{
		{c + tx * a.x, 		     tx * a.y - s * a.z, tx * a.z - s * a.y, 0},
		{    ty * a.x - a.z * s, ty * a.y + c,       ty * a.z + s * a.x, 0},
		{    tz * a.x + s * a.y, tz * a.y - s * a.x, tz * a.z + c,       0},
		{0, 0, 0, 1}
	}};
	return(r);
}

inline Mat4x4
mat4x4_ortographic_2d(f32 l, f32 r, f32 b, f32 t) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0, 0},
		{0,                 2.0f / (t - b),      0, 0},
		{0,                 0,                  -1, 0},
		{-(r + l) / (r - l), -(t + b) / (t - b), 0, 1}
	}};
	return result;
}

inline Mat4x4
mat4x4_ortographic_3d(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	Mat4x4 result =	{{
		{2.0f / (r - l),    0,                   0,                 0},
		{0,                 2.0f / (t - b),      0,                 0},
		{0,                 0,                  -2.0f / (f - n),    0},
		{-(r + l) / (r - l), -(t + b) / (t - b),-(f + n) / (f - n), 1}
	}};
	return result;
}

inline Mat4x4
mat4x4_perspective(f32 fov, f32 aspect, f32 n, f32 f) {
	const f32 toHf = tanf(fov * 0.5f);

	Mat4x4 r = {{
		{1.0f / (aspect * toHf), 0,           0,                         0},
		{0,                      1.0f / toHf, 0,                         0},
		{0,                      0,          -       (f + n) / (f - n), -1},
		{0,                      0,          -2.0f * (f * n) / (f - n),  0}
	}};
	return(r);
}

inline Mat4x4
mat4x4_mul(Mat4x4 a, Mat4x4 b) {
	Mat4x4 result;
	for(int r = 0; r < 4; ++r) {
		for(int c = 0; c < 4; ++c) {
            result.e[r][c] = a.e[0][c] * b.e[r][0]
                           + a.e[1][c] * b.e[r][1]
                           + a.e[2][c] * b.e[r][2]
                           + a.e[3][c] * b.e[r][3];
		}
	}
	return result;
}

inline Mat4x4
mat4x4_inverse(Mat4x4 m) {
    f32 coef00 = m.e[2][2] * m.e[3][3] - m.e[3][2] * m.e[2][3];
    f32 coef02 = m.e[1][2] * m.e[3][3] - m.e[3][2] * m.e[1][3];
    f32 coef03 = m.e[1][2] * m.e[2][3] - m.e[2][2] * m.e[1][3];
    f32 coef04 = m.e[2][1] * m.e[3][3] - m.e[3][1] * m.e[2][3];
    f32 coef06 = m.e[1][1] * m.e[3][3] - m.e[3][1] * m.e[1][3];
    f32 coef07 = m.e[1][1] * m.e[2][3] - m.e[2][1] * m.e[1][3];
    f32 coef08 = m.e[2][1] * m.e[3][2] - m.e[3][1] * m.e[2][2];
    f32 coef10 = m.e[1][1] * m.e[3][2] - m.e[3][1] * m.e[1][2];
    f32 coef11 = m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2];
    f32 coef12 = m.e[2][0] * m.e[3][3] - m.e[3][0] * m.e[2][3];
    f32 coef14 = m.e[1][0] * m.e[3][3] - m.e[3][0] * m.e[1][3];
    f32 coef15 = m.e[1][0] * m.e[2][3] - m.e[2][0] * m.e[1][3];
    f32 coef16 = m.e[2][0] * m.e[3][2] - m.e[3][0] * m.e[2][2];
    f32 coef18 = m.e[1][0] * m.e[3][2] - m.e[3][0] * m.e[1][2];
    f32 coef19 = m.e[1][0] * m.e[2][2] - m.e[2][0] * m.e[1][2];
    f32 coef20 = m.e[2][0] * m.e[3][1] - m.e[3][0] * m.e[2][1];
    f32 coef22 = m.e[1][0] * m.e[3][1] - m.e[3][0] * m.e[1][1];
    f32 coef23 = m.e[1][0] * m.e[2][1] - m.e[2][0] * m.e[1][1];
    
    Vec4 fac0 = { .x = coef00, .y = coef00, .z = coef02, .w = coef03 };
    Vec4 fac1 = { .x = coef04, .y = coef04, .z = coef06, .w = coef07 };
    Vec4 fac2 = { .x = coef08, .y = coef08, .z = coef10, .w = coef11 };
    Vec4 fac3 = { .x = coef12, .y = coef12, .z = coef14, .w = coef15 };
    Vec4 fac4 = { .x = coef16, .y = coef16, .z = coef18, .w = coef19 };
    Vec4 fac5 = { .x = coef20, .y = coef20, .z = coef22, .w = coef23 };
    
    Vec4 vec0 = { .x = m.e[1][0], .y = m.e[0][0], .z = m.e[0][0], .w = m.e[0][0] };
    Vec4 vec1 = { .x = m.e[1][1], .y = m.e[0][1], .z = m.e[0][1], .w = m.e[0][1] };
    Vec4 vec2 = { .x = m.e[1][2], .y = m.e[0][2], .z = m.e[0][2], .w = m.e[0][2] };
    Vec4 vec3 = { .x = m.e[1][3], .y = m.e[0][3], .z = m.e[0][3], .w = m.e[0][3] };
    
    Vec4 inv0 = v4add(v4sub(v4mul(vec1, fac0), v4mul(vec2, fac1)), v4mul(vec3, fac2));
    Vec4 inv1 = v4add(v4sub(v4mul(vec0, fac0), v4mul(vec2, fac3)), v4mul(vec3, fac4));
    Vec4 inv2 = v4add(v4sub(v4mul(vec0, fac1), v4mul(vec1, fac3)), v4mul(vec3, fac5));
    Vec4 inv3 = v4add(v4sub(v4mul(vec0, fac2), v4mul(vec1, fac4)), v4mul(vec2, fac5));
    
    const Vec4 sign_a = { .x = 1, .y =-1, .z = 1, .w = -1 };
    const Vec4 sign_b = { .x =-1, .y = 1, .z =-1, .w =  1 };
    
    Mat4x4 inverse;
    for(u32 i = 0; 
        i < 4;
        ++i) {
        inverse.e[0][i] = inv0.e[i] * sign_a.e[i];
        inverse.e[1][i] = inv1.e[i] * sign_b.e[i];
        inverse.e[2][i] = inv2.e[i] * sign_a.e[i];
        inverse.e[3][i] = inv3.e[i] * sign_b.e[i];
    }
    
    Vec4 row0 = { .x = inverse.e[0][0], .y = inverse.e[1][0], .z = inverse.e[2][0], .w = inverse.e[3][0] };
    Vec4 m0   = { .x = m.e[0][0],       .y = m.e[0][1],       .z = m.e[0][2],       .w = m.e[0][3]       };
    Vec4 dot0 = v4mul(m0, row0);
    f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
    
    f32 one_over_det = 1.0f / dot1;
    
    for (u32 i = 0; i < 16; ++i) {
        inverse.i[i] *= one_over_det;
    }
    return inverse;
}

inline Vec3
mat4x4_mul_vec3(Mat4x4 m, Vec3 v) {
    f32 a = v.e[0] * m.e[0][0] + v.e[1] * m.e[1][0] + v.e[2] * m.e[2][0] + m.e[3][0]; 
    f32 b = v.e[0] * m.e[0][1] + v.e[1] * m.e[1][1] + v.e[2] * m.e[2][1] + m.e[3][1]; 
    f32 c = v.e[0] * m.e[0][2] + v.e[1] * m.e[1][2] + v.e[2] * m.e[2][2] + m.e[3][2]; 
    f32 w = v.e[0] * m.e[0][3] + v.e[1] * m.e[1][3] + v.e[2] * m.e[2][3] + m.e[3][3]; 
    
    f32 one_over_w = 1.0f / w;
    Vec3 result = v3(a * one_over_w, b * one_over_w, c * one_over_w);
    return result;
}

inline Vec3   
mat4x4_as_3x3_mul_vec3(Mat4x4 m, Vec3 v) {
    f32 a = v.e[0] * m.e[0][0] + v.e[1] * m.e[1][0] + v.e[2] * m.e[2][0]; 
    f32 b = v.e[0] * m.e[0][1] + v.e[1] * m.e[1][1] + v.e[2] * m.e[2][1]; 
    f32 c = v.e[0] * m.e[0][2] + v.e[1] * m.e[1][2] + v.e[2] * m.e[2][2]; 
    
    Vec3 result = v3(a, b, c);
    return result;    
}

typedef struct {
    Vec3 min;
    Vec3 max;  
} Bounds3;

inline Bounds3
bounds3(Vec3 min, Vec3 max) {
    return (Bounds3) {
        .min = min,
        .max = max
    };
}

inline Bounds3 
bounds3empty(void) {
    return bounds3(v3s(INFINITY), v3s(-INFINITY));   
}

inline Bounds3
bounds3i(Vec3 v) {
    return bounds3(v, v);
}

inline Bounds3 
bounds3_join(Bounds3 a, Bounds3 b) {
    Bounds3 result;
    
    result.min.x = min32(a.min.x, b.min.x);
    result.min.y = min32(a.min.y, b.min.y);
    result.min.z = min32(a.min.z, b.min.z);
    result.max.x = max32(a.max.x, b.max.x);
    result.max.y = max32(a.max.y, b.max.y);
    result.max.z = max32(a.max.z, b.max.z);
    
    return result;    
}

inline Bounds3 
bounds3_extend(Bounds3 a, Vec3 p) {
    Bounds3 result;
    
    result.min.x = min32(a.min.x, p.x);
    result.min.y = min32(a.min.y, p.y);
    result.min.z = min32(a.min.z, p.z);
    result.max.x = max32(a.max.x, p.x);
    result.max.y = max32(a.max.y, p.y);
    result.max.z = max32(a.max.z, p.z);
    
    return result;
}

typedef union {
    struct {
        Vec3 u;
        Vec3 v;
        Vec3 w;
    };
    Vec3 e[3];
} ONB;

inline ONB
onb_from_w(Vec3 n) {
    ONB result;
    result.w = normalize(n);
    Vec3 a = (abs32(result.w.x) > 0.9f) ? v3(0, 1, 0) : v3(1, 0, 0);
    result.v = normalize(cross(result.w, a));
    result.u = cross(result.w, result.v);
    return result;
}

inline Vec3
onb_local(ONB onb, Vec3 v) {
    return v3add3(v3muls(onb.u, v.x),
                  v3muls(onb.v, v.y),
                  v3muls(onb.w, v.z));
}

typedef union {
    struct {
        f32 x, y, z, w;
    };
    Vec4 v;
    f32 e[4];
} Quat4;

#define QUAT4_IDENTITY ((Quat4) { .w = 1.0f })

inline Quat4 
q4(f32 x, f32 y, f32 z, f32 w) {
    return (Quat4) { .x = x, .y = y, .z = z, .w = w };    
}

inline Quat4 
q4euler(f32 pitch, f32 yaw, f32 roll) {
    f32 cy = cosf(yaw * 0.5f);
    f32 sy = sinf(yaw * 0.5f);
    f32 cp = cosf(pitch * 0.5f);
    f32 sp = sinf(pitch * 0.5f);
    f32 cr = cosf(roll * 0.5f);
    f32 sr = sinf(roll * 0.5f);

    return q4(sr * cp * cy - cr * sp * sy,
              cr * sp * cy + sr * cp * sy,
              cr * cp * sy - sr * sp * cy,
              cr * cp * cy + sr * sp * sy);
}

inline Quat4 
q4add(Quat4 a, Quat4 b) { 
    Quat4 result; 
    result.v = v4add(a.v, b.v); 
    return result; 
}

inline Quat4 
q4sub(Quat4 a, Quat4 b) { 
    Quat4 result; 
    result.v = v4sub(a.v, b.v); 
    return result; 
}

inline Quat4 
q4divs(Quat4 q, f32 s) { 
    Quat4 result; 
    result.v = v4divs(q.v, s); return 
    result; 
}

inline Quat4 
q4muls(Quat4 q, f32 s) { 
    Quat4 result; 
    result.v = v4muls(q.v, s); return 
    result; 
}

inline Quat4 
q4normalize(Quat4 q) { 
    Quat4 result; 
    result.v = v4normalize(q.v); 
    return result; 
}

inline f32
q4dot(Quat4 a, Quat4 b) {
    return v4dot(a.v, b.v);
}

inline Mat4x4 
mat4x4_from_quat4(Quat4 q) {
    f32 xx = q.x * q.x;    
    f32 yy = q.y * q.y;    
    f32 zz = q.z * q.z;
    f32 xy = q.x * q.y;    
    f32 xz = q.x * q.z;    
    f32 yz = q.y * q.z;    
    f32 wx = q.w * q.x;    
    f32 wy = q.w * q.y;    
    f32 wz = q.w * q.z;
    
    Mat4x4 result = MAT4X4_IDENTITIY;
    result.e[0][0] = 1 - 2 * (yy + zz);
    result.e[0][1] = 2 * (xy + wz);
    result.e[0][2] = 2 * (xz - wy);
    result.e[1][0] = 2 * (xy - wz);
    result.e[1][1] = 1 - 2 * (xx + zz);
    result.e[1][2] = 2 * (yz + wx);
    result.e[2][0] = 2 * (xz + wy);
    result.e[2][1] = 2 * (yz - wx);
    result.e[2][2] = 1 - 2 * (xx + yy);  
    return result;    
}

inline Quat4 
q4lerp(Quat4 a, Quat4 b, f32 t) {
    Quat4 result;
    
    f32 cos_theta = q4dot(a, b);
    if (cos_theta > 0.9995f) {
        result = q4normalize(q4add(q4muls(a, 1 - t), q4muls(b, t)));
    } else {
        f32 theta = acosf(clamp(cos_theta, -1, 1));
        f32 thetap = theta * t;
        Quat4 qperp = q4normalize(q4sub(b, q4muls(a, cos_theta)));
        result = q4add(q4muls(a, cosf(thetap)), q4muls(qperp, sinf(thetap)));
    }
    
    return result;
}

typedef Vec3 Color;

#define RAY_MATH_H 1
#endif
