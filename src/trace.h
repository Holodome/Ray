#if !defined(TRACE_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "image.h"
#include "perlin.h"

typedef struct { 
    u64 nfaces;
    u32 *vertices_per_face;
    u32 *vertex_indices;
    Vec3 *p;
    Vec3 *n;
    Vec2 *uv;
} PolygonMeshData;

typedef struct {
    u64 ntrig, nvert;
    u32 *tri_indices;
    Vec3 *p;
    Vec3 *n;
    Vec2 *uv;
} TriangleMeshData;

typedef struct {
    u64 bounce_count;
    u64 ray_triangle_collision_tests;
    u64 ray_triangle_collision_test_succeses;
    u64 object_collision_tests;
    u64 object_collision_test_successes;
} RayCastStatistics;

typedef struct {
    RayCastStatistics *stats;
    RandomSeries *entropy;
} RayCastData;

typedef struct {
    // Ray origin 
    Vec3 orig;
    // Direction vector
    Vec3 dir;
    // Time of ray being sent
    f32 time;
} Ray;

Ray make_ray(Vec3 orig, Vec3 dir, f32 time);
Vec3 ray_at(Ray ray, f32 t);

typedef u32 CameraType;
enum {
    CameraType_None = 0x0,
    CameraType_Perspective,
    CameraType_Orthographic,
    CameraType_Environment,
};

typedef struct {
    CameraType type;
    Vec3 orig;
    f32 time_min;
    f32 time_max;
    f32 lens_radius;
    
    Vec3 x, y, z;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
} Camera;
// @TODO finish
// @TODO make camera be moving
Camera camera_perspective(Vec3 look_from, Vec3 look_at, Vec3 v_up,
                         f32 aspect_ratio, f32 vfov,
                         f32 aperture, f32 focus_dist,
                         f32 time_min, f32 time_max);
Camera camera_orhographic(Vec3 look_from, Vec3 look_at, Vec3 v_up, 
                          f32 aspect_ratio, f32 focus_dist,
                          f32 time_min, f32 time_max);
Camera camera_environment(Vec3 look_from, Vec3 look_at, Vec3 v_up, f32 time_min, f32 time_max);

Ray camera_make_ray(Camera *camera, RandomSeries *entropy, f32 u, f32 v);

typedef struct { u64 v; } TextureHandle;

typedef u32 TextureType;
enum {
    TextureType_None = 0x0,
    TextureType_Solid,
    TextureType_Checkerboard,
    TextureType_Checkerboard3D,
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
        } checkerboard, checkerboard3d;
        struct {
            Image i;
        } image;
        struct {
            Perlin p; 
            f32 s;
        } perlin;
    };
} Texture;

typedef struct { u64 v; } MaterialHandle;

typedef u32 MaterialType;
enum {
    MaterialType_None = 0x0,
    // Describes purely specular surface
    MaterialType_Metal,
    // Describes purely diffuse surface
    MaterialType_Lambertian,
    // Describes surface that is both specular and diffuse
    MaterialType_Plastic,
    MaterialType_Dielectric,
    MaterialType_DiffuseLight,
    MaterialType_Isotropic,
};

// Material controls how rays behave when they hrec certain surface and 
// what color does this surface have at given point
typedef struct {
    MaterialType type;
    union {
        struct {
            TextureHandle albedo;
            f32 roughness; 
        } metal, plastic;
        struct {
            TextureHandle albedo;
        } lambertian, isotropic;
        struct {
            f32 ir;
        } dielectric;
        struct {
            TextureHandle emit;
            bool is_both_sided;
        } diffuse_light;
    };
} Material;

typedef struct {
    Mat4x4 o2w;
    Mat4x4 w2o;
} Transform;

#define transform_t(_t) transform_tr(_t, QUAT4_IDENTITY)
#define transform_t_euler(_t, _pitch, _yaw, _roll) transform_tr(_t, q4euler(_pitch, _yaw, _roll))
inline Transform 
transform_tr(Vec3 t, Quat4 r) { 
    Mat4x4 o2w = MAT4X4_IDENTITY;
    o2w = mat4x4_mul(o2w, mat4x4_translate(t));
    o2w = mat4x4_mul(o2w, mat4x4_from_quat4(r));
    return (Transform) {
        .o2w = o2w,
        .w2o = mat4x4_inverse(o2w)
    };
}
#define EMPTY_TRANSFORM ((Transform) { .o2w = MAT4X4_IDENTITY, .w2o = MAT4X4_IDENTITY })

typedef struct { u64 v; } ObjectHandle;

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
    
    ObjectType_Disk,
    ObjectType_Sphere,
    ObjectType_Triangle,
    ObjectType_TriangleMesh,
    
    ObjectType_ObjectList,
    ObjectType_Transform,
    ObjectType_AnimatedTransform,
    
    ObjectType_BVHNode,
    
    ObjectType_ConstantMedium,
    ObjectType_Box,
};

typedef struct Object {
    ObjectType type;
    union {
        struct {
            Vec3 p;
            Vec3 n;
            MaterialHandle mat;
            // For disk
            f32 r;
        } disk;
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
            ObjectHandle obj;
            Transform t;
            Bounds3 bounds;
        } transform;
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
            f32 time[2];
            Vec3 t[2];
            Quat4 r[2];
            Bounds3 bounds;
            ObjectHandle obj;
        } animated_transform;
        struct {
            u64 ntrig, nvert;
            u32 *tri_indices;
            Vec3 *p;
            Vec3 *n;
            Vec2 *uv;
            MaterialHandle mat;
            Bounds3 bounds;
            
            f32 surface_area;
        } triangle_mesh;
        ObjectList object_list;
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
            ObjectHandle obj;
            Vec3 o;
        } object;
        struct {
            struct PDF *p[2];
        } mix;
    };
} PDF;

// @TODO actually malloc these or whatever...
PDF cosine_pdf(Vec3 w);
PDF object_pdf(ObjectHandle obj, Vec3 orig);
PDF mixture_pdf(PDF *p1, PDF *p2);

// Packed information about collision
typedef struct {
    // Hit distance
    f32 t;
    // Point of hrec
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
inline void hit_set_normal(HitRecord *hrec, Vec3 n, Ray ray);

typedef struct {
    Vec3 specular_dir;
    bool is_specular;
    Vec3 attenuation;
    PDF  pdf;
} ScatterRecord;

typedef struct {
    // Arena used to allocate all arrays of world into.
    // Due to a excessive use of dynamic arrays, currently a lot of memory is being wasted,
    // but whatever 
    MemoryArena arena;
    // Storage of different assets in world. Use handles to get certain obj
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
    ObjectHandle important_objects;
    bool has_importance_sampling;
    // List of objects in scene
    ObjectHandle object_list;
} World;

void world_init(World *world);
// This is main function used in raycasting.
// Called from multiple threads, so everything should be thread-safe.
// Returns color of casted ray.
Vec3 ray_cast(World *world, Ray ray, i32 depth, RayCastData data);

Texture  *get_texture(World *world, TextureHandle h);
Material *get_material(World *world, MaterialHandle h);
Object *get_object(World *world, ObjectHandle h);

TextureHandle  new_texture(World *world, Texture texture);
MaterialHandle new_material(World *world, Material material);
ObjectHandle   new_object(World *world, Object obj);

ObjectHandle add_object(World *world, ObjectHandle list_handle, ObjectHandle o);
ObjectHandle add_object_to_world(World *world, ObjectHandle o);
ObjectHandle add_important_object(World *world, ObjectHandle o);

TextureHandle texture_solid(World *world, Vec3 c);
TextureHandle texture_checkerboard(World *world, TextureHandle t1, TextureHandle t2);
TextureHandle texture_checkerboard3d(World *world, TextureHandle t1, TextureHandle t2);
TextureHandle texture_image(World *world, Image i);
TextureHandle texture_perlin(World *world, Perlin perlin, f32 s);
TextureHandle texture_uv(World *world);
TextureHandle texture_normal(World *world);

MaterialHandle material_lambertian(World *world, TextureHandle albedo);
MaterialHandle material_isotropic(World *world, TextureHandle albedo);
MaterialHandle material_metal(World *world, TextureHandle albedo, f32 fuzz);
MaterialHandle material_plastic(World *world, TextureHandle albedo, f32 fuzz);
MaterialHandle material_dielectric(World *world, f32 ir);
MaterialHandle material_diffuse_light(World *world, TextureHandle t, bool is_both_sided);

ObjectHandle object_list(World *world);
ObjectHandle object_disk(World *world, Vec3 p, Vec3 n, f32 r, MaterialHandle mat);
ObjectHandle object_sphere(World *world, Vec3 p, f32 r, MaterialHandle mat);
ObjectHandle object_transform(World *world, ObjectHandle obj, Transform transform);
ObjectHandle object_triangle(World *world, Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat);
ObjectHandle object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat);
ObjectHandle object_constant_medium(World *world, f32 d, MaterialHandle phase, ObjectHandle bound);
ObjectHandle object_bvh_node(World *world, ObjectList object_list, u64 start, u64 end);
ObjectHandle object_animated_transform(World *world, ObjectHandle obj, f32 time0, f32 time1, 
                                      Vec3 t0, Vec3 t1, Quat4 r0, Quat4 r1);
#define object_triangle_mesh_p(_world, _pm, _mat) object_triangle_mesh_pt(_world, _pm, _mat, EMPTY_TRANSFORM)
#define object_triangle_mesh_t(_world, _pm, _mat) object_triangle_mesh_tt(_world, _pm, _mat, EMPTY_TRANSFORM)
ObjectHandle object_triangle_mesh_pt(World *world, PolygonMeshData  pm, MaterialHandle mat, Transform transform);
ObjectHandle object_triangle_mesh_tt(World *world, TriangleMeshData tm, MaterialHandle mat, Transform transform);
// Returns texture color for given hrec characteristics
Vec3 sample_texture(World *world, TextureHandle handle, HitRecord *hrec);
bool material_scatter(World *world, Ray ray, HitRecord hrec, ScatterRecord *scatter, RayCastData data);
Vec3 material_emit(World *world, Ray ray, HitRecord hrec, RayCastData data);

bool object_hit(World *world, Ray ray, ObjectHandle obj_handle, f32 t_min, f32 t_max, HitRecord *hrec, RayCastData data);
Bounds3 get_object_bounds(World *world, ObjectHandle obj_handle);
f32 get_object_pdf_value(World *world, ObjectHandle object_handle, Vec3 orig, Vec3 v, RayCastData data);
Vec3 get_object_random(World *world, ObjectHandle object_handle, Vec3 o, RayCastData data);

f32 pdf_value(World *world, PDF pdf, Vec3 dir, RayCastData data);
Vec3 pdf_generate(World *world, PDF pdf, RayCastData data);

// Helper functions, used for scene generation
// @TODO move them to some other place because they are not related to world construction or ray tracing
void add_xy_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 y0, f32 y1, f32 z, MaterialHandle mat);
void add_yz_rect(World *world, ObjectHandle list, f32 y0, f32 y1, f32 z0, f32 z1, f32 x, MaterialHandle mat);
void add_xz_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 z0, f32 z1, f32 y, MaterialHandle mat);
void add_box(World *world, ObjectHandle list, Vec3 p0, Vec3 p1, MaterialHandle mat);
ObjectHandle add_poly_sphere(World *world, f32 r, u32 divs, MaterialHandle mat);

#define TRACE_H 1
#endif
