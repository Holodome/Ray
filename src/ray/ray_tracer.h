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
inline Vec3 
ray_point_at(Ray ray, f32 t)
{
    Vec3 result = vec3_add(ray.origin, vec3_muls(ray.dir, t));
    return result;
}

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

inline TEXTURE_PROC(texture_proc_image)
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
        .solid_color = color,
        .type = Texture_Solid
    }; 
    return result;
}

inline Texture
texture_checkered(Vec3 checkered1, Vec3 checkered2)
{
    Texture result = {
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
    // @TODO(hl): Change to point
    f32 dist;

    u32 mat_index;
} Plane;

typedef struct {
    Vec3 normal;
    Vec3 point;
    f32 radius;
    u32 mat_index;
} Disk;

typedef struct {
    Vec3 pos;
    f32 radius;

    u32 mat_index;
} Sphere;

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
    Vec3 vertex0;
    Vec3 vertex1;
    Vec3 vertex2;
    
    Vec3 normal;
    
    u32 mat_index;
} Triangle;

// 7 predifined normals used in BVH.
// The idea is, rather than to have AABB which is built around 3 normals (x, y, z axes),
// we have more normals to pick min and max points for.
// This too simplifies surface, but does not leave so much space wasted in typical AABB
static Vec3 plane_set_normals[] = {
    { .x = 1,            .y = 0,            .z = 0          }, 
    { .x = 0,            .y = 1,            .z = 0          }, 
    { .x = 0,            .y = 0,            .z = 1          }, 
    { .x =  SQRT3_OVER3, .y =  SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x = -SQRT3_OVER3, .y =  SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x = -SQRT3_OVER3, .y = -SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x =  SQRT3_OVER3, .y = -SQRT3_OVER3, .z = SQRT3_OVER3}
};
#define BVH_NUM_PLANE_SET_NORMALS array_size(plane_set_normals)

typedef struct {
    // Min and max distances along every normal
    f32 d[BVH_NUM_PLANE_SET_NORMALS][2];
} Extents;

typedef struct OctreeNode {
    struct OctreeNode *children[8];
    Extents *data;
    u32     data_size;
    Extents extents;
    bool is_leaf;
    u8 depth;
} OctreeNode;

typedef struct {
    Box3 bounds;
    OctreeNode *root;
} Octree;

// VBH + octree structure
typedef struct {
    Extents extents;
    Octree *octree;
} BVH;

typedef struct {
    u32   triangle_count;
    Vec3 *p;
    Vec3 *n;
    Vec2 *uvs;
    u32  *tri_indices;
    u32 max_vertex_index;
    
    u32 mat_index;
    
    // Box3 bb;
    BVH bvh;
} TriangleMesh;

typedef u32 ObjectType;
enum {
    Object_Sphere,  
    Object_Plane,  
    Object_Disk,  
    Object_Triangle,  
    Object_TriangleMesh,  
};

typedef struct {
    ObjectType type;
    union {
        Sphere sphere;
        Plane  plane;
        Disk   disk;
        Triangle triangle;
        TriangleMesh triangle_mesh;  
    };
} Object;


// World that we are simulating
typedef struct Scene {
    Camera camera;

    u32 material_count;
    Material *materials;

    u32 sphere_count;
    Sphere *spheres;

    u32 plane_count;
    Plane *planes;
    
    u32 triangle_count;
    Triangle *triangles;
    
    u32 disk_count;
    Disk *disks;
    
    u32 mesh_count;
    TriangleMesh *meshes;
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
} HitRecord;

inline void 
hit_record_set_normal(HitRecord *record, Ray ray, Vec3 normal)
{
    if (!(dot(ray.dir, normal) < 0))
    {
        normal = vec3_neg(normal);
    }
    
    record->normal = normal;
}
inline bool has_hit(HitRecord record) { return (record.mat_index != 0); }

typedef struct {
    Vec3 base_color;
    f32 subsurface;
    f32 metallic;
    f32 specular;
    f32 specular_tint;
    f32 roughness;
    f32 anisotropic;
    f32 sheen;
    f32 sheen_tint;
    f32 clearcoat;
    f32 clearcoat_gloss;
} DisneyMaterial;

#define RAY_TRACER_H 1
#endif
