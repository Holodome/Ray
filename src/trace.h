#if !defined(TRACE_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "image.h"
#include "perlin.h"

typedef struct { u64 v; } TextureHandle;
typedef struct { u64 v; } MaterialHandle;
typedef struct { u64 v; } ObjectHandle;
typedef struct World World;

typedef struct {
    // Ray origin 
    Vec3 orig;
    // Direction vector
    Vec3 dir;
} Ray;

Ray make_ray(Vec3 orig, Vec3 dir);
Vec3 ray_at(Ray ray, f32 t);

typedef struct {
    Vec3 orig;
    Vec3 x, y, z;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    f32 lens_radius;
} Camera;

// Camera constructor
Camera make_camera(Vec3 look_from, Vec3 look_at, Vec3 v_up,
                   f32 aspect_ratio, f32 vfov,
                   f32 aperture, f32 focus_dist);

Ray camera_make_ray(Camera *camera, RandomSeries *entropy, f32 u, f32 v);

// Packed information about collision
typedef struct {
    // Hit distance
    f32 t;
    // Point of hit
    Vec3 p;
    // Surface normal
    Vec3 n;
    bool is_front_face;
    // UV coordinates for material sampling
    f32 u;
    f32 v;
    // Object material
    MaterialHandle mat;
} HitRecord;

// Sets normal and is_front_face
inline void hit_set_normal(HitRecord *hit, Vec3 n, Ray ray);

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

typedef struct {
    ObjectHandle *a;
    u64 size;
    u64 capacity;
} ObjectList;

ObjectList object_list_init(MemoryArena *arena, u32 reserve);
void add_object_to_list(MemoryArena *arena, ObjectList *list, ObjectHandle o);
void object_list_shrink_to_fit(MemoryArena *arena, ObjectList *list);

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
    ObjectType_FlipFace
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
            Bounds3 bounds;
        } bvh_node;
        struct {
            Bounds3 bounds;
            ObjectHandle sides_list;
        } box;
        struct {
            ObjectHandle obj;
        } flip_face;
    };
} Object;

typedef u32 PDFType;
enum {
    PDFType_None = 0x0,
    PDFType_Cosine,
    PDFType_Object,
    PDFType_Mixture,
};

typedef struct PDF {
    PDFType type;
    union {
        struct {
            ONB uvw;
        } cosine;
        struct {
            ObjectHandle object;
            Vec3 o;
        } object;
        struct {
            struct PDF *p[2];
        } mix;
    };
} PDF;

PDF cosine_pdf(Vec3 w);
PDF object_pdf(ObjectHandle obj, Vec3 orig);

typedef struct {
    Vec3 specular_dir;
    bool is_specular;
    Vec3 attenuation;
    PDF  pdf;
} ScatterRecord;

struct World {
    // Arena used to allocate all arrays of world into.
    // Due to a excessive use of dynamic arrays, currently a lot of memory is being wasted,
    // but whatever 
    MemoryArena arena;
    // Storage of different assets in world. Use handles to get certain object
    Texture *textures;
    u64 textures_size;
    u64 textures_capacity;
    Material *materials;
    u64 materials_size;
    u64 materials_capacity;
    Object *objects;
    u64 objects_size;
    u64 objects_capacity;
    // Scene settings    
    Camera camera;
    Vec3 backgorund_color;
    // Object list, objects in which are sampled directly (more rays are sent towards them) 
    ObjectHandle lights;
    // List of objects in scene
    ObjectHandle object_list;
};

typedef struct {
    u64 bounce_count;
    u64 ray_triangle_collision_tests;
    u64 object_collision_tests;
} RayCastStatistics;

// This is main function used in raycasting.
// Called from multiple threads, so everything should be thread-safe.
// Returns color of casted ray.
// Also writes some statistics to _data_
Vec3 ray_cast(World *world, Ray ray, RandomSeries *entropy, i32 depth, RayCastStatistics *stats);

inline TextureHandle  new_texture(World *world, Texture texture);
inline MaterialHandle new_material(World *world, Material material);
inline ObjectHandle   new_object(World *world, Object object);
inline Texture  *get_texture(World *world, TextureHandle h);
inline Material *get_material(World *world, MaterialHandle h);
inline Object *get_object(World *world, ObjectHandle h);

inline void add_object(World *world, ObjectHandle list_handle, ObjectHandle o);
inline void add_object_to_world(World *world, ObjectHandle o);

TextureHandle texture_solid(World *world, Vec3 c);
TextureHandle texture_checkered(World *world, TextureHandle t1, TextureHandle t2);
TextureHandle texture_image(World *world, Image i);
TextureHandle texture_perlin(World *world, Perlin perlin, f32 s);
TextureHandle texture_uv(World *world);
TextureHandle texture_normal(World *world);

MaterialHandle material_lambertian(World *world, TextureHandle albedo);
MaterialHandle material_isotropic(World *world, TextureHandle albedo);
MaterialHandle material_metal(World *world, TextureHandle albedo, f32 fuzz);
MaterialHandle material_dielectric(World *world, f32 ir);
MaterialHandle material_diffuse_light(World *world, TextureHandle t);

ObjectHandle object_list(World *world);
ObjectHandle object_flip_face(World *world, ObjectHandle obj);
ObjectHandle object_sphere(World *world, Vec3 p, f32 r, MaterialHandle mat);
ObjectHandle object_instance(World *world, ObjectHandle obj, Vec3 t, Vec3 r);
ObjectHandle object_triangle(World *world, Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat);
ObjectHandle object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat);
ObjectHandle object_constant_medium(World *world, f32 d, MaterialHandle phase, ObjectHandle bound);
ObjectHandle object_bvh_node(World *world, ObjectList object_list, u64 start, u64 end);
// Returns texture color for given hit characteristics
Vec3 sample_texture(World *world, TextureHandle handle, HitRecord *hit);
// BRDF 
bool material_scatter(World *world, RandomSeries *entropy, MaterialHandle mat, 
                      Ray ray, HitRecord hit, ScatterRecord *scatter);
f32 material_scattering_pdf(World *world, MaterialHandle mat, HitRecord hit, Ray ray);
// Returns color of emited light for given hit characteristics
Vec3 material_emit(World *world, RandomSeries *entropy, MaterialHandle mat, HitRecord hit, Ray ray);

bool get_object_bounds(World *world, ObjectHandle obj_handle, Bounds3 *box);
f32 get_object_pdf_value(World *world, RandomSeries *entropy, ObjectHandle object_handle, Vec3 orig, Vec3 v, RayCastStatistics *stats);
Vec3 get_object_random(World *world, RandomSeries *entropy, ObjectHandle object_handle, Vec3 o);
bool object_hit(World *world, ObjectHandle obj_handle, Ray ray, HitRecord *hit, f32 t_min, f32 t_max, RandomSeries *entropy, RayCastStatistics *stats);

// Helper functions, used for scene generation
// @TODO move them to some other place because they are not related to world construction or ray tracing
void add_xy_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 y0, f32 y1, f32 z, MaterialHandle mat);
void add_yz_rect(World *world, ObjectHandle list, f32 y0, f32 y1, f32 z0, f32 z1, f32 x, MaterialHandle mat);
void add_xz_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 z0, f32 z1, f32 y, MaterialHandle mat);
void add_box(World *world, ObjectHandle list, Vec3 p0, Vec3 p1, MaterialHandle mat);

#define TRACE_H 1
#endif
