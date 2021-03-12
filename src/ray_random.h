#if !defined(RAY_RANDOM_H)

#include "general.h"
#include "ray_math.h"

#define USE_XORWOW 1

typedef struct {
#if USE_XORWOW
    u32 a, b, c, d, e, counter;
#else 
    u32 state;
#endif 
} RandomSeries;

static inline u32 
xorshift32(RandomSeries *series) {
#if USE_XORWOW 
    u32 t = series->e;
    u32 s = series->a;
    series->e = series->d;
    series->d = series->c;
    series->c = series->b;
    series->b = s;
    t ^= t >> 2;
    t ^= t << 1;
    t ^= s ^ (s << 4);
    series->a = t;
    series->counter += 0x587C5;
    return t + series->counter;
#else 
    u32 x = series->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    series->state = x;
    return x;
#endif 
}

static inline void
seed_rng(RandomSeries *series, u32 seed) {
#if USE_XORWOW
    memset(series, 0, sizeof(*series));
    series->e = seed;
#else 
    series->state = seed;
#endif 
}

static inline f32 
randomu(RandomSeries *series) {
    f32 result = (f32)xorshift32(series) / ((f32)U32_MAX + 1);
    return result;
}

static inline f32 
random_bilateral(RandomSeries *series) {
    f32 result = randomu(series) * 2.0f - 1.0f;
    return result;
}

static inline f32 
random_uniform(RandomSeries *series, f32 low, f32 high) {
    f32 result = low + (high - low) * randomu(series);
    return result;
}

static inline i32
random_int(RandomSeries *rs, i32 modulo) {
    assert(modulo);
    return xorshift32(rs) % modulo;
} 

static inline i32
random_int_range(RandomSeries *rs, i32 low, i32 high) {
    assert(low != high);
    return low + (xorshift32(rs) % (u32)(high - low));
} 

static inline Vec3 
random_unit_vector(RandomSeries *rs) {
    Vec3 result = v3(random_bilateral(rs), random_bilateral(rs), random_bilateral(rs));
    return result;
}

static inline Vec3 
random_unit_sphere(RandomSeries *rs) {
    Vec3 result;
    do {
        result = random_unit_vector(rs);
    } while(length_sq(result) >= 1.0f);
    return result;
}

static inline Vec3 
random_unit_disk(RandomSeries *rs) {
    Vec3 result;
    do {
        result = v3(random_bilateral(rs), random_bilateral(rs), 0);
    } while(length_sq(result) >= 1.0f);
    return result;
}

static inline Vec3 
random_hemisphere(RandomSeries *rs, Vec3 normal) {
    Vec3 result = random_unit_sphere(rs);
    if (dot(result, normal) > 0.0f) {
        
    } else {
        result = v3neg(result);
    }
    return result;
}

static inline Vec3 
random_vector(RandomSeries *rs, f32 low, f32 high) {
    Vec3 result = v3(random_uniform(rs, low, high), 
                     random_uniform(rs, low, high),
                     random_uniform(rs, low, high));
    return result;
}

static inline Vec3
random_cosine_direction(RandomSeries *rs) {
    f32 r1 = randomu(rs);
    f32 r2 = randomu(rs);
    f32 z = sqrt32(1 - r2);
    
    f32 phi = TWO_PI * r1;
    f32 x = cosf(phi) * sqrt32(r2);
    f32 y = sinf(phi) * sqrt32(r2);
    
    return v3(x, y, z);
}

static inline Vec3 
random_to_sphere(RandomSeries *entropy, f32 r, f32 dsq) {
    f32 r1 = randomu(entropy);
    f32 r2 = randomu(entropy);
    f32 z = 1.0f + r2 * (sqrt32(1.0f - r * r / dsq) - 1);
    
    f32 phi = TWO_PI * r1;
    f32 x = cosf(phi) * sqrt32(1.0f - z * z);
    f32 y = sinf(phi) * sqrt32(1.0f - z * z);
    return v3(x, y, z);
}

extern RandomSeries rng;

#define RAY_RANDOM_H 1
#endif
