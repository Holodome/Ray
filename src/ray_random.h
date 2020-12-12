#if !defined(RAY_RANDOM_H)

#include "general.h"
#include "ray_math.h"

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

inline i32
random_int(RandomSeries *rs, i32 modulo) {
    assert(modulo);
    return xorshift32(rs) % modulo;
} 

inline i32
random_int_range(RandomSeries *rs, i32 low, i32 high) {
    assert(low != high);
    return low + (xorshift32(rs) % (u32)(high - low));
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

inline Vec3
random_cosine_direction(RandomSeries *rs) {
    f32 r1 = random(rs);
    f32 r2 = random(rs);
    f32 z = sqrt32(1 - r2);
    
    f32 phi = TWO_PI * r1;
    f32 x = cosf(phi) * sqrt32(r2);
    f32 y = sinf(phi) * sqrt32(r2);
    
    return v3(x, y, z);
}

inline Vec3 
random_to_sphere(RandomSeries *entropy, f32 r, f32 dsq) {
    f32 r1 = random(entropy);
    f32 r2 = random(entropy);
    f32 z = 1.0f + r2 * (sqrt32(1.0f - r * r / dsq) - 1);
    
    f32 phi = TWO_PI * r1;
    f32 x = cosf(phi) * sqrt32(1.0f - z * z);
    f32 y = sinf(phi) * sqrt32(1.0f - z * z);
    return v3(x, y, z);
}

extern RandomSeries global_entropy;

#define RAY_RANDOM_H 1
#endif
