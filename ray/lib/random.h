// Contains series of random number generation helper functions
//
#if !defined(RANDOM_H)

#include "lib/common.h"
#include "lib/ray_math.h"

// @NOTE(hl): Struct holding state of RNG
// State field can be used to seed the RNG to produce same sequence of numbers.
// However, rand from CRT does not use any state structure - thats because it 
// holds its state in thread local storage, which is actually the slowes part of rand
// By making our own RNG, we also have to take care of state, while also gaining massive speed increase
typedef struct {
    u32 state;
} RandomSeries;


// https://en.wikipedia.org/wiki/Xorshift
inline u32 
xorshift32(RandomSeries *series)
{
    u32 x = series->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    series->state = x;
    return x;
}

// @NOTE(hl): Generates random number in 0-1 range
inline f32 
random_unitlateral(RandomSeries *series)
{
    f32 result = (f32)xorshift32(series) / (f32)U32_MAX;    
    return result;
}

// @NOTE(hl): Generates random number in -1-1 range
inline f32 
random_bilateral(RandomSeries *series)
{
    f32 result = 2.0f * random_unitlateral(series) - 1.0f;
    return result;
}

// Generates random number in given range
inline f32 
random_range(RandomSeries *series, f32 min, f32 max)
{
	f32 rand_value = random_unitlateral(series);
	f32 result = lerp(min, max, rand_value);
	return result;
}

// Returns Vec3 where each component is a random number in range -1-1
inline Vec3
random_unit_sphere(RandomSeries *series)
{
    Vec3 result = {
        .x = random_bilateral(series),
        .y = random_bilateral(series),
        .z = random_bilateral(series)
    };
    return result;
}

#define RANDOM_H 1
#endif