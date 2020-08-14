#if !defined(RAY_H)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "ray_math.h"
#include "random.h"

#include "sys.h"
#include "ray_lane.h"

typedef struct 
{
    Vec3 origin;
    Vec3 dir;
} Ray;

// Gets point of ray with given param
// Result is addition of origin and direction multiplied by param
inline Vec3 ray_point_at(Ray ray, f32 param);

typedef struct 
{
    // Lambertian reflection
    // 0 is pure diffuse, 1 is pure specular
    // @NOTE(hl): Actually this is more 'not scatter'
    f32 scatter; 
    // Glass refraction
    // 0-1 
    // When we hit glass material, we actually should trace 2 rays: reflected (as usual diffuse) and refracted
    // It is compicated to do both of them so we set material setting of probability of refracted ray being 
    // calculated instead of reflected 
    f32 refraction_probability; 
    
    Vec3 emit_color;
    Vec3 reflect_color;
} Material;

typedef struct 
{
    Vec3 normal;
    // Distance along normal, from origin
    f32  dist;
    
    u32 mat_index;
} Plane;

typedef struct 
{
    Vec3 pos;
    f32  radius;
    
    u32 mat_index;
} Sphere;

// World that we are simulating
typedef struct 
{
    u32 material_count;
    Material *materials;
    
    u32 sphere_count;
    Sphere *spheres;
    
    u32 plane_count;
    Plane *planes;
} Scene;

//  Wrapper for 4-component image (little-endian RGBA), where each pixel is represented as u32
typedef struct 
{
    u32 width;
    u32 height;
    u32 *pixels;  
} ImageU32;

// Allocates pixel buffer
void image_u32_init(ImageU32 *image, u32 width, u32 height);
// Saves image with given filename (actually it is always PNG)
void image_u32_save(ImageU32 *image, char *filename);
u32 *image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y);

// Single work order from multithreaded rendering system
typedef struct 
{
    // Lockless data
	Scene    *scene;
    ImageU32 *image;
    // Region bounds
    u32 xmin;
    u32 ymin;
    u32 one_past_xmax;
    u32 one_past_ymax;
    // Seed should be unique per order
    RandomSeries random_series;
} RenderWorkOrder;

// Per-program work queue
typedef struct 
{
    // Total work orders
    u32              work_order_count;
    RenderWorkOrder *work_orders;
    // Incremented each time order is 'consumed'
    volatile u64 next_work_order_index;
    // Tracker for bounces
    volatile u64 bounces_computed;
    // How many orders are finished
    volatile u64 tile_retired_count; 
} RenderWorkQueue;

// Data that is passed to raycating function
typedef struct 
{
	Scene *scene;
    u32 rays_per_pixel;
    u32 max_bounce_count;
    f32 focus_distance;
    Vec3 camera_pos;
    Vec3 camera_x;
    Vec3 camera_y;
    Vec3 camera_z;
    f32 film_w;
    f32 film_h;
    f32 half_film_w;
    f32 half_film_h;
    Vec3 film_center;
    f32 half_pix_w;
    f32 half_pix_h;
    RandomSeries series;
    f32 film_x;
    f32 film_y;
    
    Vec3 final_color;
    u64  bounces_computed;
} CastState;

typedef struct 
{
    Vec3 normal;    
    u32  mat_index;
    f32  distance;
} HitRecord;

inline bool has_hit(HitRecord record) { return (record.mat_index != 0); }

#define RAY_H 1
#endif
