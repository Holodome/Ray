#if !defined(TRACE_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "image.h"
#include "perlin.h"

#include "world.h"

typedef struct {
    u64 bounce_count;
    u64 ray_triangle_collision_tests;
    u64 ray_triangle_collision_test_succeses;
    u64 object_collision_tests;
    u64 object_collision_test_successes;
    u64 russian_roulette_terminated_bounces;
} RayCastStatistics;

typedef struct {
    // Statistics of raycasting
    RayCastStatistics *stats;
    // Seeded RNG
    RandomSeries *entropy;
    // Arena where to allocate per-cast data, like PDFs 
    MemoryArena *arena;
} RayCastData;

// Packed information about collision
typedef struct {
    // Hit distance
    f32 t;
    // Point of hrec
    Vec3 p;
    // Surface normal
    Vec3 n, no;
    bool is_front_face;
    f32  ndoti, ndotio;
    // UV coordinates for material sampling
    f32 u, v;
    // Object material
    MaterialHandle mat;
    ObjectHandle   obj;
} HitRecord;

// Sets normal and is_front_face
inline void hit_set_normal(HitRecord *hrec, Vec3 n, Ray ray);

typedef struct {
    // Direction of scattered ray
    Vec3 dir;
    // mat_color * bsdf * cos_theta
    Vec3 bsdf;
    // pdf 
    f32  pdf;
    // mat_color * bsdf * cos_theta / pdf
    Vec3 weight;
} ScatterRecord;

// This is main function used in raycasting.
// Called from multiple threads, so everything should be thread-safe.
// Returns color of casted ray.
Vec3 ray_cast(World *world, Ray ray, i32 depth, RayCastData data);

Vec3 sample_texture(World *world, TextureHandle handle, HitRecord *hrec);
Vec3 material_emit(World *world, Ray ray, HitRecord hrec, RayCastData data);

bool object_hit(World *world, Ray ray, ObjectHandle obj_handle, f32 t_min, f32 t_max, HitRecord *hrec, RayCastData data);
Bounds3 get_object_bounds(World *world, ObjectHandle obj_handle);
// f32 get_object_pdf_value(World *world, ObjectHandle object_handle, Vec3 orig, Vec3 v, RayCastData data);
// Returns random point inside object
// Vec3 get_object_random(World *world, ObjectHandle object_handle, Vec3 o, RayCastData data, ObjectHandle *a);

#define TRACE_H 1
#endif
