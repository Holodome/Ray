#if !defined(RAY_TRACER_H)

#include "lib/common.h"
#include "lib/ray_math.h"
#include "lib/random.h"
#include "image.h"

typedef struct {
    Vec3 origin;
    Vec3 dir;

    f32 time;
} Ray;

inline Ray ray(Vec3 origin, Vec3 dir, f32 time);
// Gets point of ray with given param
// Result is addition of origin and direction multiplied by param
inline Vec3 ray_point_at(Ray ray, f32 param);

struct Texture;

#define TEXTURE_PROC(name) Vec3 name(struct Texture *texture, Vec2 uv, Vec3 point)
typedef TEXTURE_PROC(TextureProc);

typedef u8 TextureType;
enum {
    Texture_Solid,
    Texture_Checkered,
    Texture_Image
};

typedef struct Texture {
    TextureType type;
    TextureProc *proc;
    union {
        Vec3 solid_color;
        struct 
        {
            ImageU32 image;
            char *filename;
        };
        struct {
            Vec3 checkered1;
            Vec3 checkered2;
        };
    };
} Texture;

TEXTURE_PROC(texture_proc_solid_color) { return texture->solid_color; }
TEXTURE_PROC(texture_proc_checkered)     
{
    Vec3 result;
    
    // @TODO(hl): -1 - 1 range of abs parameter is mirrored. Find other algorithm
    if ((mod32(abs32(point.x + 10000.0f), 2.0f) > 1.0f) - (mod32(abs32(point.y + 10000.0f), 2.0f) > 1.0f))
    {
        result = texture->checkered1;
    }
    else 
    {
        result = texture->checkered2;
    }
    
    return result;
}
TEXTURE_PROC(texture_proc_image)
{
    Vec3 result;
    
    ImageU32 *image = &texture->image;
    
    if (!image->pixels)
    {
        result = vec3(1, 0, 1);
    }
    else 
    {
        uv.u = 1.0f - clamp(uv.u, 0, 1);
        uv.v = 1.0f - clamp(uv.v, 0, 1);
        
        u32 x = round32(uv.u * image->width);
        u32 y = round32(uv.v * image->height);
        
        if (x >= image->width) 
        {
            x = image->width - 1;
        }
        if (y >= image->height)
        {
            y = image->height - 1;
        }
        
        f32 r255 = reciprocal32(255.0f);
        u8 *pixels = (u8 *)image_u32_get_pixel_pointer(image, x, y);
        
        result.x = pixels[0] * r255;
        result.y = pixels[1] * r255;
        result.z = pixels[2] * r255;
    }
    
    return result;
}

inline Texture 
texture_solid_color(Vec3 color) 
{ 
    Texture result = { 
        .proc = texture_proc_solid_color,
        .solid_color = color,
        .type = Texture_Solid
    }; 
    return result;
}

inline Texture
texture_checkered(Vec3 checkered1, Vec3 checkered2)
{
    Texture result = {
        .proc = texture_proc_checkered,
        .checkered1 = checkered1,
        .checkered2 = checkered2,
        .type = Texture_Checkered
    };
    return result;
}

inline Texture
texture_image(char *filename)
{
    Texture result = {
        .proc = texture_proc_image,
        .filename = filename,
        .type = Texture_Image
    };
    image_u32_load(&result.image, filename);
    return result;
}

typedef struct {
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
    // Vec3 reflect_color;
    Texture texture;
} Material;

typedef struct {
    Vec3 normal;
    // Distance along normal, from origin
    f32 dist;

    u32 mat_index;
} Plane;

typedef struct {
    Vec3 pos;
    f32 radius;

    u32 mat_index;
} Sphere;

typedef struct {
    Vec3 center0;
    Vec3 center1;
    f32 time0;
    f32 time1;
    f32 radius;
    
    u32 mat_index;
} MovingSphere;

inline Vec3 moving_sphere_center(MovingSphere *sphere, f32 time);

typedef struct {
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

    f32 time0;
    f32 time1;
} Camera;

// Returns ray from cemera center aimed on position on film
inline Ray camera_ray_at(Camera *camera, f32 film_x, f32 film_y, RandomSeries *series);
Camera camera(Vec3 pos, ImageU32 *image);

typedef struct {
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
    f32 k;
    u32 mat_index;
} XYRect; 

typedef struct {
    f32 x0;
    f32 z0;
    f32 x1;
    f32 z1;
    f32 k;
    u32 mat_index;
} XZRect; 

typedef struct {
    f32 y0;
    f32 z0;
    f32 y1;
    f32 z1;
    f32 k;
    u32 mat_index;
} YZRect; 

typedef u8 RectType;
enum
{
    RectType_XY,
    RectType_XZ,
    RectType_YZ
};

typedef struct {
    RectType type;
    union 
    {
        XYRect xy;
        XZRect xz;
        YZRect yz;
    };
    
    f32 rotation_y;
} Rect;

typedef struct {
    Vec3 vertex0;
    Vec3 vertex1;
    Vec3 vertex2;
    
    u32 mat_index;
} Triangle;

// World that we are simulating
typedef struct Scene {
    Camera camera;

    u32 material_count;
    Material *materials;

    u32 sphere_count;
    Sphere *spheres;

    u32 plane_count;
    Plane *planes;
    
    u32 moving_sphere_count;
    MovingSphere *moving_spheres;
    
    u32 rect_count;
    Rect *rects;
    
    u32 triangle_count;
    Triangle *triangles;
} Scene;

// Data that is passed to raycating function
typedef struct {
    Scene *scene;
    u32 rays_per_pixel;
    u32 max_bounce_count;

    RandomSeries series;
    f32 film_x;
    f32 film_y;

    Vec3 final_color;
    u64 bounces_computed;
} CastState;

typedef struct {
    Vec3 normal;
    u32 mat_index;
    f32 distance;
    // UV coordinates of hit position
    Vec2 uv;
    Vec3 hit_point;
    
    bool front_face;
} HitRecord;

inline void 
hit_record_set_normal(HitRecord *record, Ray ray, Vec3 normal)
{
    bool front_face = true;
    if (!(vec3_dot(ray.dir, normal) < 0))
    {
        front_face = false;
        normal = vec3_neg(normal);
    }
    
    record->front_face = front_face;
    record->normal = normal;
}
inline bool has_hit(HitRecord record) { return (record.mat_index != 0); }

#define RAY_TRACER_H 1
#endif
