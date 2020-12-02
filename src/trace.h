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

//
// Objects
//

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

typedef struct {
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
        struct {
            ObjectHandle *objects;
            u64 objects_size;
            u64 objects_capacity;
        } object_list;
        struct {
            ObjectHandle object;
            Mat4x4 o2w;
            Mat4x4 w2o;
        } instance;
        struct {
            ObjectHandle left;
            ObjectHandle right;
            Box3 box;
        } bvh_node;
        struct {
            Box3 box;
            ObjectHandle sides_list;
        } box;
    };
} Object;

#define OBJECT_LIST ( (Object) { .type = ObjectType_ObjectList } )

Object object_sphere(Vec3 p, f32 r, MaterialHandle mat);
Object object_instance(ObjectHandle obj, Vec3 t, Vec3 r);
Object object_triangle(Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat);
Object object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat);
// @TODO replace objects list with some kind of structure
Object object_bvh_node(World *world, ObjectHandle *objects_init, u64 objects_total_size,
                       u64 start, u64 end);

// Gets bounding box for given object and writes it to box. If object has no AABB, return false
bool object_get_box(World *world, ObjectHandle obj_handle, Box3 *box);

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

// Dynamic array hacks
#define DEFAULT_ARRAY_CAPACITY 10
#define IAMLAZYTOINITWORLD(_a, _arr, _capacity) { if (!_capacity) { _capacity = DEFAULT_ARRAY_CAPACITY; _arr = arena_alloc(_a, sizeof(*_arr) * _capacity); } }
#define EXPAND_IF_NEEDED(_a, _arr, _size, _capacity) { IAMLAZYTOINITWORLD(_a, _arr, _capacity);  \
    if (_size + 1 > _capacity) { _arr = arena_realloc(_a, _arr, sizeof(*_arr) * _capacity, sizeof(*_arr) * _capacity * 2); _capacity *= 2; } }
#define SHRINK_TO_FIT(_a, _arr, _size, _capacity) { _arr = arena_realloc(_a, _arr, sizeof(*_arr) * _capacity, sizeof(*_arr) * _size); _capacity = _size; }

// #define IAMLAZYTOINITWORLD(_a, _arr, _capacity) { if (!_capacity) { _capacity = 10; _arr = malloc(sizeof(*_arr) * _capacity); } }
// #define EXPAND_IF_NEEDED(_a, _arr, _size, _capacity) { IAMLAZYTOINITWORLD(_a, _arr, _capacity);  \
//     if (_size + 1 > _capacity) { _arr = realloc(_arr, sizeof(*_arr) * _capacity * 2); _capacity *= 2; } }
// #define SHRINK_TO_FIT(_a, _arr, _size, _capacity) { _arr = realloc(_arr, sizeof(*_arr) * _size); _capacity = _size; }


// Adds some kind of resource to world
TextureHandle add_texture(World *world, Texture texture);
MaterialHandle add_material(World *world, Material material);
ObjectHandle add_object(World *world, Object object);

void  
add_object_to_list(World *world, ObjectHandle list_handle, ObjectHandle object) {
    Object *list = world->objects + list_handle.v;
    assert(list->type = ObjectType_ObjectList);
    
    EXPAND_IF_NEEDED(&world->arena, list->object_list.objects, list->object_list.objects_size, list->object_list.objects_capacity);
    
    list->object_list.objects[list->object_list.objects_size++] = object;
}

void 
object_list_shrink_to_fit(World *world, ObjectHandle list_handle) {
    Object *list = world->objects + list_handle.v;
    assert(list->type == ObjectType_ObjectList);
    
    SHRINK_TO_FIT(&world->arena, list->object_list.objects, list->object_list.objects_size, list->object_list.objects_capacity);
}

void 
add_xy_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 y0, f32 y1, f32 z, MaterialHandle mat) {
    Vec3 v00 = v3(x0, y0, z);
    Vec3 v01 = v3(x0, y1, z);
    Vec3 v10 = v3(x1, y0, z);
    Vec3 v11 = v3(x1, y1, z);
    
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v01, v11, mat)));
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v11, v10, mat)));
}

void 
add_yz_rect(World *world, ObjectHandle list, f32 y0, f32 y1, f32 z0, f32 z1, f32 x, MaterialHandle mat) {
    Vec3 v00 = v3(x, y0, z0);
    Vec3 v01 = v3(x, y0, z1);
    Vec3 v10 = v3(x, y1, z0);
    Vec3 v11 = v3(x, y1, z1);
    
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v01, v11, mat)));
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v11, v10, mat)));
}

void 
add_xz_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 z0, f32 z1, f32 y, MaterialHandle mat) {
    Vec3 v00 = v3(x0, y, z0);
    Vec3 v01 = v3(x0, y, z1);
    Vec3 v10 = v3(x1, y, z0);
    Vec3 v11 = v3(x1, y, z1);
    
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v01, v11, mat)));
    add_object_to_list(world, list, add_object(world, object_triangle(v00, v11, v10, mat)));
}

void 
add_box(World *world, ObjectHandle list, Vec3 p0, Vec3 p1, MaterialHandle mat) {
    add_xy_rect(world, list, p0.x, p1.x, p0.y, p1.y, p1.z, mat);
    add_xy_rect(world, list, p0.x, p1.x, p0.y, p1.y, p0.z, mat);
    add_xz_rect(world, list, p0.x, p1.x, p0.z, p1.z, p1.y, mat);
    add_xz_rect(world, list, p0.x, p1.x, p0.z, p1.z, p0.y, mat);
    add_yz_rect(world, list, p0.y, p1.y, p0.z, p1.z, p1.x, mat);
    add_yz_rect(world, list, p0.y, p1.y, p0.z, p1.z, p0.x, mat);
}

typedef struct {
    u64 bounce_count;
    u64 ray_triangle_collision_tests;
    u64 object_collision_tests;
} RayCastData;

// This is main function used in raycasting.
// Called from multiple threads, so everything should be thread-safe.
// Returns color of casted ray.
// Also writes some statistics to _data_
Vec3 ray_cast(World *world, Ray ray, RandomSeries *entropy, RayCastData *data);

#define TRACE_H 1
#endif
