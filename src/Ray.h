#if !defined(RAY_H)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Common.h"
#include "RayMath.h"
#include "Random.h"

#include "Sys.h"

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
    f32 scatter; // 0 is pure diffuse, 1 is pure specular
    Vec4 emit_color;
    Vec4 reflect_color;
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

} World;

//  Wrapper for 4-component image, where each pixel is represented as u32
typedef struct 
{
    u32 width;
    u32 height;
    u32 *pixels;  
} ImageU32;

ImageU32 image_u32_make(u32 width, u32 height);
void     image_u32_save(ImageU32 *image, char *filename);
u32     *image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y);

typedef struct 
{
    World *world;
    ImageU32 *image;
    u32 xmin;
    u32 ymin;
    u32 one_past_xmax;
    u32 one_past_ymax;
    
    RandomSeries random_series;
} RenderWorkOrder;

typedef struct 
{
    u32 work_order_count;
    RenderWorkOrder *work_orders;
    
    volatile u64 next_work_order_index;
    volatile u64 bounces_computed;
    volatile u64 tile_retired_count; 
} RenderWorkQueue;

#define RAY_H 1
#endif
