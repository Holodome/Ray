#if !defined(GENERAL_H)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <time.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define U32_MAX UINT32_MAX

#define CT_ASSERT(_expr) static_assert(_expr, #_expr)

//
// RNG
//

typedef struct {
    u32 state;
} RandomSeries;

inline u32 
xorshift32(RandomSeries *series) {
    u32 x = series->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    series->state = x;
    return x;
}

inline f32 
random(RandomSeries *series) {
    f32 result = (f32)xorshift32(series) / ((f32)U32_MAX + 1);
    return result;
}

inline f32 
random_bilateral(RandomSeries *series) {
    f32 result = random(series) * 2.0f - 1.0f;
    return result;
}

inline f32 
random_uniform(RandomSeries *series, f32 low, f32 high) {
    f32 result = low + (high - low) * random(series);
    return result;
}

inline i64
random_int(RandomSeries *rs, i64 low, i64 high) {
    return (i64)floorf(random_uniform(rs, low, high + 1));
} 

//
// Math 
//

#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define HALF_PI 1.57079632679f

#define DEG2RAD(_deg) ((_deg) * PI / 180.0f)

inline f32 
lerp(f32 a, f32 b, f32 t) {
    // assert(0 <= t && t <= 1);
    return (1.0f - t) * a + t * b;
}

inline f32 
clamp(f32 x, f32 low, f32 high) {
    if (x < low) {
        x = low;
    }
    if (x > high) {
        x = high;
    }
    return x;
}

inline f32 
rsqrtf(f32 a) {
    return 1.0f / sqrtf(a);
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
    f32 result = sqrtf(length_sq(a));
    return result;
}

inline Vec3 
normalize(Vec3 a)
{
    Vec3 result = v3muls(a, rsqrtf(length_sq(a)));
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
    const f32 epsilon = 1e-8;
    bool result = (fabsf(a.x) < epsilon) && (fabsf(a.y) < epsilon) && (fabsf(a.z) < epsilon);
    return result;
}

// 
// Misc helpers
// 

inline u32 
rgba_pack_4x8(u32 r, u32 g, u32 b, u32 a) {
    // If values passed here are greater that 255 something for sure went wrong
    assert(!(r & ~0xFFu) && !(g & ~0xFFu) && !(b & ~0xFFu) && !(a & ~0xFFu));
    u32 result = r << 0 | g << 8 | b << 16 | a << 24;
    return result;
}

inline u32
rgba_pack_4x8_linear1(f32 r, f32 g, f32 b, f32 a) {
    u32 ru = roundf(clamp(r, 0, 0.999f) * 255.0f);
    u32 gu = roundf(clamp(g, 0, 0.999f) * 255.0f);
    u32 bu = roundf(clamp(b, 0, 0.999f) * 255.0f);
    u32 au = roundf(clamp(a, 0, 0.999f) * 255.0f);
    u32 result = rgba_pack_4x8(ru, gu, bu, au);
    return result;
}


inline Vec3 
random_unit_vector(RandomSeries *rs) {
    Vec3 result = v3(random_bilateral(rs), random_bilateral(rs), random_bilateral(rs));
    return result;
}

inline Vec3 
random_unit_sphere(RandomSeries *rs) {
    Vec3 result;
    do {
        result = random_unit_vector(rs);
    } while(length_sq(result) >= 1.0f);
    return result;
}

inline Vec3 
random_unit_disk(RandomSeries *rs) {
    Vec3 result;
    do {
        result = v3(random_bilateral(rs), random_bilateral(rs), 0);
    } while(length_sq(result) >= 1.0f);
    return result;
}

inline Vec3 
random_hemisphere(RandomSeries *rs, Vec3 normal) {
    Vec3 result = random_unit_sphere(rs);
    if (dot(result, normal) > 0.0f) {
        
    } else {
        result = v3neg(result);
    }
    return result;
}

inline Vec3 
random_vector(RandomSeries *rs, f32 low, f32 high) {
    Vec3 result = v3(random_uniform(rs, low, high), 
                     random_uniform(rs, low, high),
                     random_uniform(rs, low, high));
    return result;
}

inline f32 
trilerp(f32 c[2][2][2], f32 u, f32 v, f32 w) {
    f32 accum = 0;
    for (i32 i = 0;
         i < 2;
         ++i) {
        for (i32 j = 0;
             j < 2;
             ++j) {
            for (i32 k = 0;
                 k < 2;
                 ++k) {
                f32 a = (i * u + (1 - i) * (1 - u)) *
                        (j * v + (1 - j) * (1 - v)) *
                        (k * w + (1 - k) * (1 - w));
                accum += a * c[i][j][k];     
            }     
        }        
    }
    
    return accum;
}

#define MAX_BOUNCE_COUNT  8llu
#define SAMPLES_PER_PIXEL 1024llu

#define GENERAL_H 1
#endif
