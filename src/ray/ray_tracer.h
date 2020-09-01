#if !defined(RAY_TRACER_H)

#include "lib/common.h"
#include "lib/ray_math.h"
#include "lib/random.h"
#include "lib/memory_arena.h"

#include "image.h"

//
// Ray
//

typedef struct {
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
};

// @TODO(hl): Think if we can use texture indices instead of pointers
typedef struct Texture {
    TextureType type;
    union {
		struct {
			Vec3 color;
		} solid;
		struct {
			struct Texture *texture1;
			struct Texture *texture2;
			
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
            char *filename;
        } image;
    };
} Texture;

// Texture initialization functions
void texture_init_solid(Texture *texture, Vec3 c);
void texture_init_scale(Texture *texture, Texture *texture1, Texture *texture2);
void texture_init_mix(Texture *texture, Texture *texture1, Texture *texture2, f32 mix);
void texture_init_bilerp(Texture *texture, Vec3 c00, Vec3 c01, Vec3 c10, Vec3 c11);
void texture_init_uv(Texture *texture);
void texture_init_checkered(Texture *texture, Texture *texture1, Texture *texture2);
void texture_init_image(Texture *texture, char *filename);

// Returns texture color in given position
Vec3 sample_texture(Texture *texture, Vec2 uv, Vec3 hit_point);

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
    
    Texture *texture;
} Material;

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

typedef struct {
    f32 radius;
    f32 height;
    
    Vec3 normal;
    Vec3 p;
} Cylinder;

typedef struct {
    f32 radius;
    f32 height;
    
    Vec3 normal;
    Vec3 p;
} Cone;

typedef struct {
} Paraboloid;

typedef struct {
} Hyperboloid;

//
// BVH
// @TODO(hl): Cleanup

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
    f32 dmin[BVH_NUM_PLANE_SET_NORMALS];
    f32 dmax[BVH_NUM_PLANE_SET_NORMALS];
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
typedef struct {
    Mat4x4 o2w;
    Mat4x4 w2o;
} Transform;

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

inline Transform
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

typedef struct {
    ObjectType type;
    u32 mat_index;
    Transform transform;
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

// @TODO(hl): Move mat_index parameter to front
void object_init_sphere(Object *object, Transform transform, Sphere sphere, u32 mat_index);
void object_init_plane(Object *object, Transform transform, Plane plane, u32 mat_index);
void object_init_disk(Object *object, Transform transform, Disk disk, u32 mat_index);
void object_init_triangle(Object *object, Transform transform, Triangle triangle, u32 mat_index);
void object_init_cylinder(Object *object, Transform transform, Cylinder cylinder, u32 mat_index);
void object_init_cone(Object *object, Transform transform, Cone cone, u32 mat_index);
void object_init_hyperboloid(Object *object, Transform transform, Hyperboloid hyperboloid, u32 mat_index);
void object_init_paraboloid(Object *object, Transform transform, Paraboloid paraboloid, u32 mat_index);
void object_init_triangle_mesh(Object *object, Transform transform, TriangleMesh mesh, u32 mat_index);

//
// Scene
//

typedef struct Scene {
	MemoryArena arena;
    
    u32 material_count;
    Material *materials;
	
    u32 object_count;
    Object *objects;
    
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
    u32 mat_index;
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
inline bool has_hit(HitRecord record) { return (record.mat_index != 0); }

#define RAY_TRACER_H 1
#endif
