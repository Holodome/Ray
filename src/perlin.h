#if !defined(PERLIN_H)

#include "general.h"

#define PERLIN_POINT_COUNT 256 

typedef struct {
    Vec3 *ranvec;
    i32 *perm_x;
    i32 *perm_y;
    i32 *perm_z;
} Perlin;

static void 
perlin_permute(RandomSeries *rs, i32 *p, u32 n) {
    for (i32 i = n - 1; 
         i > 0;
         --i) {
        i32 target = random_int(rs, 0, i);
        i32 tmp = p[i];
        p[i] = p[target];
        p[target] = tmp; 
    }
}

static i32 *
perlin_generate_perm(MemoryArena *arena, RandomSeries *rs) {
    i32 *p = arena_alloc(arena, PERLIN_POINT_COUNT * sizeof(i32));
    
    for (u32 i = 0;
         i < PERLIN_POINT_COUNT;
         ++i) {
        p[i] = i;       
    }
    perlin_permute(rs, p, PERLIN_POINT_COUNT);
    
    return p;
}

Perlin 
make_perlin(MemoryArena *arena, RandomSeries *rs) {
    Perlin perlin = {0};
    
    perlin.ranvec = calloc(PERLIN_POINT_COUNT, sizeof(Vec3));
    for (u32 i = 0;
         i < PERLIN_POINT_COUNT;
         ++i) {
        perlin.ranvec[i] = normalize(random_vector(rs, -1, 1));        
    }
    
    perlin.perm_x = perlin_generate_perm(arena, rs);
    perlin.perm_y = perlin_generate_perm(arena, rs);
    perlin.perm_z = perlin_generate_perm(arena, rs);
    
    return perlin;
}

f32 
perlin_interp(Vec3 c[2][2][2], f32 u, f32 v, f32 w) {
    f32 uu = u * u * (3 - 2 * u);
    f32 vv = v * v * (3 - 2 * v);
    f32 ww = w * w * (3 - 2 * w);
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
                f32 a = (i * uu + (1 - i) * (1 - uu)) *
                        (j * vv + (1 - j) * (1 - vv)) *
                        (k * ww + (1 - k) * (1 - ww));
                Vec3 weight_v = v3(u - i, v - j, w - k);
                accum += a * dot(c[i][j][k], weight_v);     
            }     
        }        
    }
    
    return accum;
}

f32 
perlin_noise(Perlin *perlin, Vec3 p) {
    i32 i = (i32)floorf(p.x);
    i32 j = (i32)floorf(p.y);
    i32 k = (i32)floorf(p.z);
    Vec3 c[2][2][2];
    
    for (i32 di = 0; 
         di < 2;
         ++di) {
        for (i32 dj = 0;
             dj < 2;
             ++dj) {
            for (i32 dk = 0;
                 dk < 2;
                 ++dk) {
                c[di][dj][dk] = perlin->ranvec[perlin->perm_x[(i + di) & 0xFF] ^ 
                                               perlin->perm_y[(j + dj) & 0xFF] ^ 
                                               perlin->perm_z[(k + dk) & 0xFF]];
            }     
        }        
    }
    
    f32 u = p.x - floorf(p.x);
    f32 v = p.y - floorf(p.y);
    f32 w = p.z - floorf(p.z);
    
    f32 result = perlin_interp(c, u, v, w);
    // return (result + 1) * 0.5f
    return result;
}

f32 
perlin_turb(Perlin *perlin, Vec3 p, u32 octaves) {
    f32 accum = 0;
    Vec3 temp_p = p;
    f32 weight = 1.0f;
    
    for (u32 octave = 0;
         octave < octaves;
         ++octave) {
        accum += weight * perlin_noise(perlin, temp_p);
        weight *= 0.5f;
        temp_p = v3muls(temp_p, 2);        
    }
    
    return fabsf(accum);
}

#define PERLIN_H 1
#endif
