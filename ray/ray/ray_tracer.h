#if !defined(RAY_TRACER_H)

#include "lib/common.h"
#include "lib/ray_math.h"
#include "lib/random.h"
#include "lib/memory_arena.h"

#include "image.h"

#define INVALID_ID U32_MAX
#define is_valid_id(id_) ((id_).id != INVALID_ID)

//
// Ray
//

// Structure containing data about single ray, shot from camera or later bounced from some surface
typedef struct {
    // 
    Vec3 origin;
    // Normalized direction vector
    Vec3 direction;
    // Inverse direction vector. Used to save time in some computations
    Vec3 inverse_direction;
    
    // Space-time raytracing ray 'time'
    f32 time;
} Ray;

inline Ray 
make_ray(Vec3 origin, Vec3 dir, f32 time)
{
    Ray result = {
        .origin = origin,
        .direction = dir,
        .inverse_direction = vec3_neg(dir),
        .time = time
    };
    return result;
}

inline void 
ray_update_dir(Ray *ray, Vec3 dir)
{
    ray->direction = dir;
    ray->inverse_direction = vec3_neg(dir);
}
// Gets point of ray with given param
// Result is addition of origin and direction multiplied by param
inline Vec3 
ray_point_at(Ray ray, f32 t)
{
    Vec3 result = vec3_add(ray.origin, vec3_muls(ray.direction, t));
    return result;
}

//
// Camera
//

typedef struct {
    // @TODO(hl): Use matrix for this???
    Vec3 camera_pos;
    Vec3 camera_x;
    Vec3 camera_y;
    Vec3 camera_z;
    // 
    
    f32 film_w;
    f32 film_h;
    f32 half_film_w;
    f32 half_film_h;
    Vec3 film_center;
    f32 half_pix_w;
    f32 half_pix_h;
	// Range of time for ray creation
    f32 time0;
    f32 time1;
} Camera;

// Returns ray from cemera center aimed on position on film
inline Camera make_camera(Vec3 pos, ImageU32 *image);
inline Ray camera_ray_at(Camera *camera, f32 film_x, f32 film_y, RandomSeries *series);


//
// Texture interface.
//

// Texture is essentially a function that returns color of given point on surface,
// so textures can be thought about as lambdas. However, calling function is perfomance-critical,
// so in places, where ne need to get color from texture, we do swtich (texture_type) instead.
typedef u8 TextureType;
enum {
	// Returns constant color 
    Texture_Solid,
	// Returns color of two textures, multiplied
	Texture_Scale,
	// Returns mix of two textures
	Texture_Mix,
	// Returns interpolation between 4 constant values
	Texture_BilinearInterpolation,
	// Returns UV coordinates represented as red and green
	Texture_UV,
	// Returns value from procedural checkerboard pattern 
    Texture_Checkered,
	// Returns sample color from image
    Texture_Image,
	// ...
	Texture_PerlinNoise,
    // Sets resulting color be equal to surface normal of hit in given point\
    // @TODO implement
    Texture_Normal,
};

// Some data is not essential for texture to function, but is useful when saving and loading scenes
// This meta data is put to separate structure not to be confused with useful info
typedef union {
    struct {
        char *filename;
    } image;
} TextureMetaData;

typedef struct {
    u32 id;
} TextureID;

// @TODO(hl): Think if we can use texture indices instead of pointers
typedef struct Texture {
    TextureType type;
    union {
		struct {
			Vec3 color;
		} solid;
		struct {
			TextureID texture1;
			TextureID texture2;
			
			f32 mix_value;
		} scale, mix, checkered;
		struct {
			Vec3 color00;
			Vec3 color01;
			Vec3 color10;
			Vec3 color11;
		} bilinear_interpolation;
        struct {
            ImageU32 image;
        } image;
    };
    
    TextureMetaData meta;
} Texture;

// @NOTE(hl): It is kinda stupid that here are several structures array-like structures
// They all use ids to operate
// So it may be wiser, if we were in C++, to use generics here but whatever 
typedef struct {
    Texture *textures;
    u32      count;
    u32      max_count;
} TextureArray;

// Returns texture color in given position
Vec3 sample_texture(TextureArray *textures, TextureID id, Vec2 uv, Vec3 hit_point);

// These functions create new texture from given parameters and append it to texture array if it has enough space
// Id of texture is returned, which can be used in creating materails
// @TODO(hl): Change this to single append functtion and move texture constructors to other place
TextureID textures_append_solid    (TextureArray *textures, Vec3 c);
TextureID textures_append_scale    (TextureArray *textures, TextureID texture1, TextureID texture2);
TextureID textures_append_mix      (TextureArray *textures, TextureID texture1, TextureID texture2, f32 mix);
TextureID textures_append_bilerp   (TextureArray *textures, Vec3 c00, Vec3 c01, Vec3 c10, Vec3 c11);
TextureID textures_append_uv       (TextureArray *textures);
TextureID textures_append_checkered(TextureArray *textures, TextureID texture1, TextureID texture2);
TextureID textures_append_image    (TextureArray *textures, char *filename);


//
// Materials
//

// Material, that every object in scene has
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
    
    TextureID texture;
} Material;

typedef struct {
    u32 id;
} MaterialID;

typedef struct {
    Material *materials;
    u32       count;
    u32       max_count;
} MaterialArray;

MaterialID materials_append(MaterialArray *materials, Material material);

//
// Scene objects
//

typedef struct {
    Vec3 normal;
    // Distance along normal, from origin
    // @TODO(hl): Change to point
    f32 dist;
} Plane;

typedef struct {
    Vec3 normal;
    Vec3 point;
    f32 radius;
} Disk;

typedef struct {
    // Vec3 pos;
    f32 radius;
} Sphere;

typedef struct {
    Vec3 vertex0;
    Vec3 vertex1;
    Vec3 vertex2;
    
    Vec3 normal;
} Triangle;

// @TODO
typedef struct {
    f32 radius;
    f32 height;
    
    Vec3 normal;
    Vec3 p;
} Cylinder;

// @TODO
typedef struct {
    f32 radius;
    f32 height;
    
    Vec3 normal;
    Vec3 p;
} Cone;

// @TODO
typedef struct {
} Paraboloid;

// @TODO
typedef struct {
} Hyperboloid;

//
// BVH
// @TODO(hl): Cleanup

// 7 predifined normals used in BVH.
// The idea is, rather than to have AABB which is built around 3 normals (x, y, z axes),
// we have more normals to pick min and max points for.
// This too simplifies surface, but does not leave so much space wasted in typical AABB
static const Vec3 BVH_PLANE_SET_NORMALS[] = {
    { .x = 1,            .y = 0,            .z = 0          }, 
    { .x = 0,            .y = 1,            .z = 0          }, 
    { .x = 0,            .y = 0,            .z = 1          }, 
    { .x =  SQRT3_OVER3, .y =  SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x = -SQRT3_OVER3, .y =  SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x = -SQRT3_OVER3, .y = -SQRT3_OVER3, .z = SQRT3_OVER3}, 
    { .x =  SQRT3_OVER3, .y = -SQRT3_OVER3, .z = SQRT3_OVER3}
};
#if 1
#define BVH_PLANE_SET_NORMALS_COUNT array_size(BVH_PLANE_SET_NORMALS)
#else 
// @NOTE(hl): For testing. This way we have 3-normal bounding box (which is simply AABB)
#define BVH_PLANE_SET_NORMALS_COUNT 3 
#endif 

typedef struct {
    // Min and max distances along every normal
    f32 dmin[BVH_PLANE_SET_NORMALS_COUNT];
    f32 dmax[BVH_PLANE_SET_NORMALS_COUNT];
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
    // @TODO(hl): Implement octree
    // Octree *octree;
} BVH;

typedef struct {
    u32   triangle_count;
    Vec3 *p;
    Vec3 *n;
    Vec2 *uvs;
    u32  *tri_indices;
    u32 max_vertex_index;
    
    BVH bvh;
} TriangleMesh;

//
// Transform
//

// @TODO(hl): Make transforms be able to scale (may need to change intersection algorithms)
// Holds object-to-world and its inverse (world-to-object) matrix.
// Basically only o2s could do, but we store its inverse to speed up some computations where we need to make it
typedef struct {
    Mat4x4 o2w;
    Mat4x4 w2o;
} Transform;

// Animated transform strcuture for space-time raytracing
typedef struct {
    // Supplied via constructor call
    Transform start_transform;
    Transform end_transform;
    f32       start_time;
    f32       end_time;
    // Decomposed from original data
    Vec3   t[2];
    Quat4  r[2];
    Mat4x4 s[2];
} DynamicTransform;

inline void 
decompose(Mat4x4 m, Vec3 t[2], Quat4 r[2], Mat4x4 s[2])
{
    
}

inline DynamicTransform
dynamic_transform(Transform start_transform,
                  Transform end_transform,
                  f32       start_time,
                  f32       end_time)
{
    DynamicTransform result = {};
    result.start_transform = start_transform;
    result.end_transform = end_transform;
    result.start_time = start_time;
    result.end_time = end_time;
    
    
    
    return result;
}

#define empty_transform() ( (Transform){ .o2w = mat4x4_identity(), .w2o = mat4x4_identity() } )
inline Transform 
make_transform_o2w(Mat4x4 object_to_world)
{
    Transform result = {
        .o2w = object_to_world,
        .w2o = mat4x4_inverse(object_to_world)
    };
    return result; 
}

Transform
make_transform(Vec3 translation, Vec3 rotation)
{
    // Operations are done in SRT order
    Mat4x4 o2w = mat4x4_identity();
    o2w = mat4x4_mul(o2w, mat4x4_rotate(rotation.x, vec3(1, 0, 0)));
    o2w = mat4x4_mul(o2w, mat4x4_rotate(rotation.y, vec3(0, 1, 0)));
    o2w = mat4x4_mul(o2w, mat4x4_rotate(rotation.z, vec3(0, 0, 1)));
    o2w = mat4x4_mul(o2w, mat4x4_translate(translation));
    
    Transform result = make_transform_o2w(o2w);
    return result;    
}

inline Transform 
make_transform_translate(Vec3 translation)
{
    Transform result = make_transform_o2w(mat4x4_translate(translation));
    return result;
}

//
// Objects
//

typedef u8 ObjectType;
enum {
    Object_Sphere,  
    Object_Plane,  
    Object_Disk,  
    Object_Triangle,  
    Object_Cylinder,
    Object_Cone,
    Object_Hyperboloid,
    Object_Paraboloid,
    Object_TriangleMesh,  
};

typedef union {
    struct {
        bool is_from_file;
        char *filename;
    } triangle_mesh;
} ObjectMetaData;

typedef struct {
    ObjectType type;
    MaterialID mat_id;
    Transform transform;
    ObjectMetaData meta;
    union {
        Sphere sphere;
        Plane  plane;
        Disk   disk;
        Triangle triangle;
        Cylinder cylinder;
        Cone cone;
        Hyperboloid hyperboloid;
        Paraboloid paraboloid;
        TriangleMesh triangle_mesh;  
    };
} Object;

Object make_object_sphere(Transform transform, MaterialID mat_id, Sphere sphere);
Object make_object_plane(Transform transform, MaterialID mat_id, Plane plane);
Object make_object_disk(Transform transform, MaterialID mat_id, Disk disk);
Object make_object_triangle(Transform transform, MaterialID mat_id, Triangle triangle);
Object make_object_cylinder(Transform transform, MaterialID mat_id, Cylinder cylinder);
Object make_object_cone(Transform transform, MaterialID mat_id, Cone cone);
Object make_object_hyperboloid(Transform transform, MaterialID mat_id, Hyperboloid hyperboloid);
Object make_object_paraboloid(Transform transform, MaterialID mat_id, Paraboloid paraboloid);
Object make_object_triangle_mesh(Transform transform, MaterialID mat_id, TriangleMesh mesh);

typedef struct {
    u32 id;
} ObjectID;

typedef struct {
    Object *objects;
    u32     count;
    u32     max_count;
} ObjectArray;

ObjectID objects_append(ObjectArray *objects, Object object);

//
// Scene
//

typedef struct {
    // Memory arena used to allocate all scene stuff. This way to free scene we can only free arena,
    // and do not deallocate each array separately
	MemoryArena arena;
    // Asset arrays
    TextureArray  textures;
    MaterialArray materials;
	
    ObjectArray objects;
    
    Camera camera;
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
    MaterialID mat_id;
    f32 distance;
    // UV coordinates of hit position
    Vec2 uv;
    Vec3 hit_point;
    bool is_front_face;
} HitRecord;

inline void 
hit_record_set_normal(HitRecord *record, Ray ray, Vec3 normal)
{
    record->is_front_face = true;
    
    if (!(dot(ray.direction, normal) < 0))
    {
        record->is_front_face = false;
        normal = vec3_neg(normal);
    }
    
    record->normal = normal;
}
inline bool has_hit(HitRecord record) { return (is_valid_id(record.mat_id)); }

#define RAY_TRACER_H 1
#endif
