#if !defined(TRACE_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "image.h"
#include "perlin.h"

//
// See World definition.
// This is pointer-like handles to objects in world
typedef struct { u64 v; } TextureHandle;
typedef struct { u64 v; } MaterialHandle;
typedef struct { u64 v; } ObjectHandle;
typedef struct World World;

//
// Helper functions
//

inline Vec3 
reflect(Vec3 v, Vec3 n) {
    Vec3 result = v3sub(v, v3muls(n, 2.0f * dot(v, n)));
    return result;
}

inline Vec3 
refract(Vec3 v, Vec3 n, f32 etai_over_etat) {
    f32 cos_theta = fminf(dot(v3neg(v), n), 1.0f);
    Vec3 r_out_perp = v3muls(v3add(v, v3muls(n, cos_theta)), etai_over_etat);
    Vec3 r_out_parallel = v3muls(n, -sqrtf(fabsf(1.0f - length_sq(r_out_perp))));
    Vec3 result = v3add(r_out_perp, r_out_parallel);
    return result; 
}

inline f32
schlick(f32 cosine, f32 ref_idx) {
    f32 r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    f32 result = r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
    return result;
}

inline void 
sphere_get_uv(Vec3 p, f32 *u, f32 *v) {
#if 1
    f32 theta = acosf(-p.y);
    f32 phi = atan2f(-p.z, p.x) + PI;
    
    *u = phi / TWO_PI;
    *v = theta / PI;
#else 
    f32 phi = atan2f(p.y, p.x);
    f32 theta = asinf(p.z);
    
    *u = 1.0f - (phi + PI) / TWO_PI;
    *v = (theta + HALF_PI) / PI;
#endif 
}


//
// Ray
//

typedef struct {
    Vec3 orig;
    Vec3 dir;
} Ray;

inline Ray 
make_ray(Vec3 orig, Vec3 dir) {
    return (Ray) {
        .orig = orig,
        .dir = dir
    };
}

inline Vec3 
ray_at(Ray ray, f32 t) {
    Vec3 result = v3add(ray.orig, v3muls(ray.dir, t));
    return result;
}

//
// Camera
//

// Perspective projection camera
typedef struct {
    Vec3 orig;
    // Orthonormal basis
    Vec3 x;
    Vec3 y;
    Vec3 z;
    
    f32 lens_radius;
    
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
} Camera;

// Camera constructor
Camera make_camera(Vec3 look_from, Vec3 look_at, Vec3 v_up,
                   f32 aspect_ratio, f32 vfov,
                   f32 aperture, f32 focus_dist);

Ray camera_make_ray(Camera *camera, RandomSeries *entropy, f32 u, f32 v);

//
// Hit record
//

typedef struct {
    f32 t;
    Vec3 p;
    Vec3 n;
    bool is_front_face;
    f32 u;
    f32 v;
    
    MaterialHandle mat;
} HitRecord;

static void 
hit_set_normal(HitRecord *hit, Vec3 n, Ray ray) {
    if (dot(ray.dir, n) < 0.0f) {
        hit->is_front_face = true;
        hit->n = n;
    } else {
        hit->is_front_face = false;
        hit->n = v3neg(n);
    }
}

//
// Textures
//

typedef u32 TextureType;
enum {
    TextureType_None = 0x0,
    TextureType_Solid,
    TextureType_Checkered,
    TextureType_Image,
    TextureType_Perlin,
    TextureType_UV,
    TextureType_Normal,
};

// Texture is essentially a function that returns some color with given 
// hit characteristics
typedef struct {
    TextureType type;
    union {
        struct {
            Vec3 c;
        } solid;
        struct {
            TextureHandle t1;
            TextureHandle t2;
        } checkered;
        struct {
            Image i;
        } image;
        struct {
            Perlin p; 
            f32 s;
        } perlin;
    };
} Texture;

// Texture constructors
inline Texture
texture_solid(Vec3 c) {
    return (Texture) {
        .type = TextureType_Solid,
        .solid = {
            .c = c
        }
    };
}

inline Texture
texture_perlin(Perlin perlin, f32 s) {
    return (Texture) {
        .type = TextureType_Perlin,
        .perlin = {
            .p = perlin,
            .s = s
        }
    };
}
// Recursive texture sampler
Vec3 sample_texture(World *world, TextureHandle handle, HitRecord *hit);

//
// Metarial
//

typedef u32 MaterialType;
enum {
    MaterialType_None = 0x0,
    MaterialType_Metal,
    MaterialType_Lambertian,
    MaterialType_Dielectric,
    MaterialType_DiffuseLight,
    MaterialType_Isotropic,
};

// Material controls how rays behave when they hit certain surface and 
// what color does this surface have at given point
typedef struct {
    MaterialType type;
    union {
        struct {
            TextureHandle albedo;
            f32 fuzz; 
        } metal;
        struct {
            TextureHandle albedo;
        } lambertian, isotropic;
        struct {
            f32 ir;
        } dielectric;
        struct {
            TextureHandle emit;
        } diffuse_light;
    };
} Material;

// Material constructors
inline Material
material_lambertian(TextureHandle tex) {
    return (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = tex
        }
    };
}

inline Material
material_isotropic(TextureHandle tex) {
    return (Material) {
        .type = MaterialType_Isotropic,
        .isotropic = {
            .albedo = tex
        }
    };
}

inline Material
material_dielectric(f32 ir) {
    return (Material) {
        .type = MaterialType_Dielectric,
        .dielectric = {
            .ir = ir
        }
    };
}

inline Material 
material_metal(TextureHandle albedo, f32 fuzz) {
    return (Material) {
        .type = MaterialType_Metal,
        .metal = {
            .albedo = albedo,
            .fuzz = fuzz
        }
    };
}

//
// Objects
//

typedef struct {
    ObjectHandle *a;
    u64 size;
    u64 capacity;
} ObjectList;

typedef u32 ObjectType;
enum {
    ObjectType_None = 0x0,
    ObjectType_ObjectList,
    ObjectType_Sphere,
    ObjectType_Triangle,
    ObjectType_ConstantMedium,
    ObjectType_Instance,
    ObjectType_BVHNode,
    ObjectType_Box,
};

typedef struct Object {
    ObjectType type;
    union {
        struct {
            Vec3 p;  
            f32  r; 
            MaterialHandle mat;
        } sphere;
        struct {
            Vec3 p[3];
            Vec3 n;
            MaterialHandle mat;
        } triangle;
        struct {
            ObjectHandle boundary;
            MaterialHandle phase_function;
            f32 neg_inv_density;
        } constant_medium;
        ObjectList object_list;
        struct {
            ObjectHandle object;
            Mat4x4 o2w;
            Mat4x4 w2o;
            Bounds3 bounds;
        } instance;
        struct {
            ObjectHandle left;
            ObjectHandle right;
            Bounds3 box;
        } bvh_node;
        struct {
            Bounds3 box;
            ObjectHandle sides_list;
        } box;
    };
} Object;

#define OBJECT_LIST ( (Object) { .type = ObjectType_ObjectList } )

Object object_sphere(Vec3 p, f32 r, MaterialHandle mat);
Object object_instance(World *world, ObjectHandle obj, Vec3 t, Vec3 r);
Object object_triangle(Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat);
Object object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat);
Object object_constant_medium(f32 d, MaterialHandle phase, ObjectHandle bound);
// @NOTE: Object list is passed by value because it is not changed
Object object_bvh_node(World *world, ObjectList object_list, u64 start, u64 end);

void add_object_to_list(MemoryArena *arena, ObjectList *list, ObjectHandle o);
void object_list_shrink_to_fit(MemoryArena *arena, ObjectList *list);

//
// World 
//

struct World {
    // Arena used to allocate all arrays of world into.
    // Due to a excessive use of dynamic arrays, currently a lot of memory is being wasted,
    // but whatever 
    MemoryArena arena;
    
    Camera camera;
    
    Vec3 backgorund_color;
    
    Texture *textures;
    u64 textures_size;
    u64 textures_capacity;

    Material *materials;
    u64 materials_size;
    u64 materials_capacity;
    
    Object *objects;
    u64 objects_size;
    u64 objects_capacity;
    
    ObjectHandle object_list;
};

inline Object *get_object(World *world, ObjectHandle h);
bool object_get_box(World *world, ObjectHandle obj_handle, Bounds3 *box);

// Adds some kind of resource to world
inline TextureHandle  new_texture(World *world, Texture texture);
inline MaterialHandle new_material(World *world, Material material);
inline ObjectHandle   new_object(World *world, Object object);

inline void add_object(World *world, ObjectHandle list_handle, ObjectHandle object);
#define add_object_to_world(_world, _obj) add_object(_world, _world->object_list, _obj)

inline void add_new_object(World *world, ObjectHandle list_handle, Object object);
#define add_new_object_to_world(_world, _obj) add_new_object(_world, _world->object_list, _obj)

void add_xy_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 y0, f32 y1, f32 z, MaterialHandle mat);
void add_yz_rect(World *world, ObjectHandle list, f32 y0, f32 y1, f32 z0, f32 z1, f32 x, MaterialHandle mat);
void add_xz_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 z0, f32 z1, f32 y, MaterialHandle mat);
void add_box(World *world, ObjectHandle list, Vec3 p0, Vec3 p1, MaterialHandle mat);

typedef struct {
    u64 bounce_count;
    u64 ray_triangle_collision_tests;
    u64 object_collision_tests;
} RayCastData;

// Settings of ray casting
// @NOTE This better should be passed as arguments, but let it be global for now 
extern u32 max_bounce_count;
extern u32 rays_per_pixel;

// This is main function used in raycasting.
// Called from multiple threads, so everything should be thread-safe.
// Returns color of casted ray.
// Also writes some statistics to _data_
Vec3 ray_cast(World *world, Ray ray, RandomSeries *entropy, RayCastData *data);

#define TRACE_H 1
#endif
