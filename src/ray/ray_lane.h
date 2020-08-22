#if !defined(RAYLANE_H)

#if 0

#include "common.h"
#include "ray_math.h"

#include <xmmintrin.h>

#if !defined(LANE_WIDTH)
#define LANE_WIDTH 1
#endif 

// This is compile-time option for compiling ray tracer in different modes
// Modern cpus allow SIMD intruction, that perform operations on multiple data components at the same time
// So algorithm is written in operator-overloading way, where we can eaasilly change different lane widths
// This way we can compile same algorithm on different cpus, allowing more modern ones to benefin in speed
//
// Writing mathematical operators in function calls can take some time get used to, but actually this is pretty straightforward

#if (LANE_WIDTH == 1)

typedef u32  LaneU32;
typedef f32  LaneF32;
typedef Vec3 LaneVec3;

#define lane_f32(a)           ((LaneF32)(a))
#define lane_f32_neg(a)       (-(a))
#define lane_f32_add(a, b)    ((a) + (b)) 
#define lane_f32_sub(a, b)    ((a) - (b))
#define lane_f32_mul(a, b)    ((a) * (b))
#define lane_f32_div(a, b)    ((a) / (b))
#define lane_f32_sqrt(a)      sqrt32(a)
#define lane_f32_from_lane_u32(a) ((LaneF32)a)
#define lane_f32_lt(a, b)     lane_u32((a) < (b))
#define lane_f32_gt(a, b)     lane_u32((a) > (b))
#define lane_f32_max(a, b)    max(a, b)
#define lane_f32_min(a, b)    min(a, b)

#define lane_u32(a)           ((LaneU32)(a))
#define lane_u32_neg(a)       (-(a))
#define lane_u32_add(a, b)    ((a) + (b)) 
#define lane_u32_sub(a, b)    ((a) - (b))
#define lane_u32_mul(a, b)    ((a) * (b))
#define lane_u32_div(a, b)    ((a) / (b))
#define lane_u32_mod(a, b)    ((a) % (b))
#define lane_u32_not(a)       (~(a))
#define lane_u32_and(a, b)    ((a) & (b))
#define lane_u32_or(a, b)     ((a) | (b))
#define lane_u32_xor(a, b)    ((a) ^ (b))
#define lane_u32_shl(a, b)    ((a) << (b))
#define lane_u32_shr(a, b)    ((a) >> (b))
#define lane_u32_bool(a)      (a)
#define lane_u32_from_lane_u32(a) ((LaneU32)a)
#define lane_u32_eq(a, b)     lane_u32((a) == (b))
#define lane_u32_lt(a, b)     lane_u32((a) < (b))
#define lane_u32_gt(a, b)     lane_u32((a) > (b))
#define lane_u32_horizontal_add(a) (a)
#define lane_u32_all_zero(a)  ((a) == 0)

#define lane_vec3(s)         (s)
#define lane_vec3s(s)         vec3s(s)
#define lane_vec3_neg(a)      vec3_neg(a)
#define lane_vec3_add(a, b)   vec3_add(a, b)
#define lane_vec3_sub(a, b)   vec3_sub(a, b)
#define lane_vec3_mul(a, b)   vec3_mul(a, b)
#define lane_vec3_div(a, b)   vec3_div(a, b)
#define lane_vec3_dot(a, b)   dot(a, b)
#define lane_cross(a, b) cross(a, b)
#define lane_vec3_normalize(a) normalize(a)

#elif (LANE_WIDTH == 4)

typedef __m128i LaneU32;
typedef __m128  LaneF32;
typedef struct {
    LaneF32 x;
    LaneF32 y;
    LaneF32 z;
} LaneVec3;

#elif (LANE_WIDTH == 8)

typedef __m256i LaneU32;
typedef __m256  LaneF32;
typedef struct {
    LaneF32 x;
    LaneF32 y;
    LaneF32 z;
} LaneVec3;

#else 
#error "Incorrect LANE_WIDTH defined (possible: 1, 4, 8)"
#endif 

// Produces branchless assignment like `if (mask) *dest = value; `
// Most times in simple cases (where we don't use SIMD) compiler is able to produce same code 
// However, just to be sure in perfomance-critical places we do this
inline void 
conditional_assign_u32(LaneU32 *dest, LaneU32 mask, LaneU32 value)
{
    // mask = (mask ? U32_MAX : 0);
    // *dest = ((~mask & *dest) | (mask & value));
    mask = (lane_u32_bool(mask) ? lane_u32(U32_MAX) : lane_u32(0));
    *dest = lane_u32_or(lane_u32_and(lane_u32_not(mask), *dest), lane_u32_and(mask, value));
}

inline void 
conditional_assign_f32(LaneF32 *dest, LaneU32 mask, LaneF32 value)
{
    conditional_assign_u32((LaneU32 *)dest, mask, *(LaneU32 *)&value);
}

inline void 
conditional_assign_vec3(LaneVec3 *dest, LaneU32 mask, LaneVec3 value)
{
    conditional_assign_f32(&dest->x, mask, value.x);
    conditional_assign_f32(&dest->y, mask, value.y);
    conditional_assign_f32(&dest->z, mask, value.z);
}


static void 
cast_sample_rays(CastState *state)
{
    World *world = state->world;
    LaneU32 rays_per_pixel = state->rays_per_pixel;
    LaneU32 max_bounce_count = state->max_bounce_count;
    LaneVec3 camera_pos = state->camera_pos;
    LaneVec3 camera_x = state->camera_x;
    LaneVec3 camera_y = state->camera_y;
    LaneVec3 camera_z = state->camera_z;
    LaneF32 film_w = state->film_w;
    LaneF32 film_h = state->film_h;
    LaneF32 half_film_w = state->half_film_w;
    LaneF32 half_film_h = state->half_film_h;
    LaneVec3 film_center = state->film_center;
    LaneF32 half_pix_w = state->half_pix_w;
    LaneF32 half_pix_h = state->half_pix_h;
    RandomSeries series = state->series;
    LaneF32 film_x = state->film_x + half_pix_w;
    LaneF32 film_y = state->film_y + half_pix_h;
    
    LaneU32 bounces_computed = lane_u32(0);
    LaneVec3 final_color = vec3(0, 0, 0);
    
    LaneU32 lane_width = LANE_WIDTH;
    LaneU32 lane_ray_count = rays_per_pixel / lane_width;
    LaneF32 contrib = reciprocal32(rays_per_pixel);
    for (u32 ray_index = 0;
        ray_index < lane_ray_count;
        ++ray_index)
    {
        LaneF32 off_x = film_x + random_bilateral(&series) * half_pix_w;
        LaneF32 off_y = film_y + random_bilateral(&series) * half_pix_h;
        LaneVec3 film_pos = vec3_add(film_center,
            vec3_add(vec3_muls(camera_x, off_x * half_film_w),
                vec3_muls(camera_y, off_y * half_film_h)));
                
        LaneVec3 ray_origin = camera_pos;
        LaneVec3 ray_dir    = normalize(vec3_sub(film_pos, camera_pos));

        LaneVec3 ray_cast_color = lane_vec3s(0);
        LaneVec3 attenuation    = lane_vec3s(1);

        LaneF32 min_hit_distance = lane_f32(0.0001f);
        LaneF32 tolerance        = lane_f32(0.0001f);

        LaneU32 lane_mask        = lane_u32(U32_MAX);

        for (u32 bounce_count = 0;
            bounce_count < 8;
            ++bounce_count)
        {
            LaneU32 lane_increment = lane_u32(1);
            bounces_computed = lane_u32_add(bounces_computed, lane_u32_and(lane_increment, lane_mask));

            LaneF32 hit_distance = lane_u32(F32_MAX);
            LaneU32 hit_mat_index = lane_u32(0);
            LaneVec3 next_normal = {0};

            for (u32 plane_index = 0;
                plane_index < world->plane_count;
                ++plane_index)
            {
                Plane plane = world->planes[plane_index];

                LaneF32  plane_d       = lane_f32(plane.dist);
                LaneVec3 plane_n       = lane_vec3(plane.normal);
                LaneU32  plane_mat_idx = lane_u32(plane.mat_index);

                LaneF32 denominator = lane_vec3_dot(plane_n, ray_dir);
                LaneF32 t = lane_f32_div(lane_f32_sub(lane_f32_neg(plane_d), lane_vec3_dot(plane_n, ray_origin)), denominator);
                
                LaneU32 denom_mask = lane_u32_gt(lane_f32_lt(denominator, lane_f32_neg(tolerance)), lane_f32_gt(denominator, tolerance));
                LaneU32 t_mask     = lane_u32_and(lane_f32_gt(t, min_hit_distance), lane_f32_lt(t, hit_distance));
                LaneU32 hit_mask   = lane_u32_and(denom_mask, t_mask);
                
                conditional_assign_f32(&hit_distance,  hit_mask, t);
                conditional_assign_u32(&hit_mat_index, hit_mask, plane_mat_idx);
                conditional_assign_vec3(&next_normal,  hit_mask, plane_n);
            }

            for (u32 sphere_index = 0;
                sphere_index < world->sphere_count;
                ++sphere_index)
            {
                Sphere sphere = world->spheres[sphere_index];

                LaneF32  sphere_r = lane_f32(sphere.radius);
                LaneVec3 sphere_p = lane_vec3(sphere.pos);
                LaneU32  sphere_mat_idx = lane_u32(sphere.mat_index);

                LaneVec3 sphere_relative_ray_origin = lane_vec3_sub(ray_origin, sphere_p);
                LaneF32 a = lane_f32(lane_vec3_dot(ray_dir, ray_dir));
                LaneF32 b = lane_f32_mul(lane_f32(2.0f), lane_vec3_dot(sphere_relative_ray_origin, ray_dir));
                LaneF32 c = lane_f32_sub(lane_vec3_dot(sphere_relative_ray_origin, sphere_relative_ray_origin), lane_f32_mul(sphere_r, sphere_r));

                LaneF32 denominator = lane_f32_mul(lane_f32(2.0f), a);
                LaneF32 root_term   = lane_f32_sqrt(lane_f32_sub(lane_f32_mul(b, b), lane_f32_mul(lane_f32(4.0f), lane_f32_mul(a, c))));
                LaneF32 tp = lane_f32_div(lane_f32_add(lane_f32_neg(b), root_term), denominator);
                LaneF32 tn = lane_f32_div(lane_f32_sub(lane_f32_neg(b), root_term), denominator);

                LaneU32 root_mask = lane_f32_gt(root_term, tolerance);

                LaneF32 t = tp;
                LaneU32 pick_mask = lane_u32_and(lane_f32_gt(tn, min_hit_distance), lane_f32_lt(tn, tp));
                conditional_assign_f32(&t, pick_mask, tn);

                LaneU32 t_mask = lane_u32_and(lane_f32_gt(t, min_hit_distance), lane_f32_lt(t, hit_distance));
                LaneU32 hit_mask = lane_u32_and(root_mask, t_mask);
                
                conditional_assign_f32(&hit_distance,  hit_mask, t);
                conditional_assign_u32(&hit_mat_index, hit_mask, sphere_mat_idx);
                conditional_assign_vec3(&next_normal,  hit_mask, lane_vec3_normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray_dir, hit_distance))));
            }

            if (hit_mat_index)
            {
                Material mat = world->materials[hit_mat_index];

                ray_cast_color  = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));
                f32 cos_atten = dot(vec3_neg(ray_dir), next_normal);
                if (cos_atten < 0) cos_atten = 0;
                attenuation = vec3_mul(attenuation, vec3_muls(mat.reflect_color, cos_atten));

                ray_origin = lane_vec3_add(ray_origin, lane_vec3_mul(ray_dir, lane_vec3s(hit_distance)));

                Vec3 pure_bounce = vec3_reflect(ray_dir, next_normal);
                Vec3 random_bounce = normalize(vec3_add(next_normal,
                    vec3(random_bilateral(&series), random_bilateral(&series), random_bilateral(&series))));
                ray_dir = normalize(vec3_lerp(random_bounce, pure_bounce, mat.scatter));
            }
            else
            {
                Material mat = world->materials[hit_mat_index];
                ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));
                break;
            }

            // Material mat = world->materials[hit_mat_index];
            
            // LaneVec3 mat_emit_color = lane_vec3(mat.emit_color);
            // LaneVec3 mat_ref_color  = lane_vec3(mat.reflect_color);
            // LaneF32  mat_scatter    = lane_f32(mat.scatter);
            
            // ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));

            // lane_mask = lane_u32_and(lane_mask, lane_u32_eq(hit_mat_index, lane_u32(0)));

            // LaneF32 cos_atten = lane_f32_max(lane_f32(0), lane_vec3_dot(lane_vec3_neg(ray_dir), next_normal));
            // attenuation = lane_vec3_mul(attenuation, vec3_muls(mat.reflect_color, cos_atten));

            // ray_origin = lane_vec3_add(ray_origin, lane_vec3_mul(ray_dir, lane_vec3s(hit_distance)));
            

            // LaneVec3 pure_bounce = vec3_reflect(ray_dir, next_normal);
            // LaneVec3 random_bounce = normalize(vec3_add(next_normal,
            //     vec3(random_bilateral(&series), random_bilateral(&series), random_bilateral(&series))));
            // ray_dir = normalize(vec3_lerp(random_bounce, pure_bounce, mat.scatter));
            
            // if(lane_u32_eq(lane_mask, lane_u32(0))) break;
        }

        final_color = vec3_add(final_color, vec3_muls(ray_cast_color, contrib));
    }

    final_color = vec3_muls(vec3(linear_to_srgb(final_color.x), 
                                 linear_to_srgb(final_color.y),
                                 linear_to_srgb(final_color.z)), 255.0f);
    state->bounces_computed += lane_u32_horizontal_add(bounces_computed);
    state->final_color = final_color;
}


// https://en.wikipedia.org/wiki/Xorshift
inline LaneU32 
xor_shift32(RandomSeries *series)
{
    LaneU32 x = series->state;
    
    x = lane_u32_xor(x, lane_u32_shl(x, 13));
    x = lane_u32_xor(x, lane_u32_shr(x, 17));
    x = lane_u32_xor(x, lane_u32_shl(x, 5 ));
    series->state = x;
    return x;
}

// @NOTE(hl): Generates random number in 0-1 range
inline LaneF32 
random_unitlateral(RandomSeries *series)
{
    LaneF32 result = lane_f32_div(lane_f32_from_lane_u32(xor_shift32(series)), lane_f32(U32_MAX));
    return result;
}

// @NOTE(hl): Generates random number in -1-1 range
inline LaneF32 
random_bilateral(RandomSeries *series)
{
    LaneF32 result = lane_f32_sub(lane_f32_mul(random_unitlateral(series), lane_f32(2.0f)), lane_f32(1.0f));
    return result;
}

#endif 

#define RAYLANE_H 1
#endif
