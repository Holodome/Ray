#if !defined(RAY_H)

#include <stdio.h>

#include "Common.h"
#include "RayMath.h"

#include "Thirdparty/stb_image_write.h"

typedef struct 
{
    Vec3 a;
    Vec3 b;
} Ray;

typedef struct 
{
    Vec4 color;
} Material;

typedef struct 
{
    Vec3 normal;
    f32  dist;
    
    u32 mat_index;
} Plane;

typedef struct 
{
    Vec3 pos;
    f32  normal;
    
    u32 mat_index;
} Sphere;

typedef struct 
{
    u32 material_count;
    Material *materials;
    
    u32 sphere_count;
    Sphere *spheres;
    
    u32 plane_count;
    Plane *planes;
} World;

// @NOTE(hl): Wrapper for 4-component image, where each pixel is represented as u32
typedef struct 
{
    u32 width;
    u32 height;
    u32 *pixels;  
} ImageU32;

ImageU32 image_u32_make(u32 width, u32 height);
void     image_u32_save(ImageU32 *image, char *filename);

#define RAY_H 1
#endif
