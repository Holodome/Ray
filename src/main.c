#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define CT_ASSERT(_expr) static_assert(_expr, #_expr)

inline f32 
random_unitlateral() {
    f32 result = (f32)rand() / ((f32)RAND_MAX + 1);
    return result;
}

inline f32 
random_bilateral() {
    f32 result = random_unitlateral() * 2.0f - 1.0f;
    return result;
}

inline f32 
lerp(f32 a, f32 b, f32 t) {
    assert(0 <= t && t <= 1);
    return (1.0f - t) * a + t * b;
}

inline f32 
rsqrtf(f32 a) {
    return 1.0f / sqrtf(a);
}

inline u32 
rgba_pack_4x8(u32 r, u32 g, u32 b, u32 a) {
    // If values passed here are greater that 255 something for sure went wrong
    assert(!(r & ~0xFFu) && !(g & ~0xFFu) && !(b & ~0xFFu) && !(a & ~0xFFu));
    u32 result = r << 0 | g << 8 | b << 16 | a << 24;
    return result;
}

inline u32
rgba_pack_4x8_linear1(f32 r, f32 g, f32 b, f32 a) {
    // assert(r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f);
    u32 ru = roundf(r * 255.0f);
    u32 gu = roundf(g * 255.0f);
    u32 bu = roundf(b * 255.0f);
    u32 au = roundf(a * 255.0f);
    u32 result = rgba_pack_4x8(ru, gu, bu, au);
    return result;
}

typedef union {
    struct {
        f32 x, y, z;  
    };
    struct {
        f32 r, g, b;  
    };
    f32 e[3];
} Vec3;

inline Vec3 
vec3(f32 x, f32 y, f32 z) {
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline Vec3 
vec3s(f32 s) {
    Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

inline Vec3 
vec3_neg(Vec3 a) {
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

inline Vec3 
vec3_add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline Vec3 
vec3_sub(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline Vec3 
vec3_div(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

inline Vec3 
vec3_mul(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

inline Vec3 
vec3_divs(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

inline Vec3 
vec3_muls(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

inline f32 
dot(Vec3 a, Vec3 b) {
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

inline Vec3 
cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

inline f32 
length_sq(Vec3 a) {
    f32 result = dot(a, a);
    return result;
}

inline f32 
length(Vec3 a) {
    f32 result = sqrtf(length_sq(a));
    return result;
}

inline Vec3 
normalize(Vec3 a)
{
    Vec3 result = vec3_muls(a, rsqrtf(dot(a, a)));
    return result;
}

inline Vec3 
vec3_lerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

#pragma pack(push, 1)

typedef struct {
    u16 file_type;
    u32 file_size;
    u32 reserved;
    u32 bitmap_offset;
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    i32 horz_resolution;
    i32 vert_resolution;
    u32 colors_used;
    u32 colors_important;
} BMPHeader;

#pragma pack(pop)

static bool 
save_bmp(char *filename, u32 width, u32 height, void *data) {
    bool result = true;
    
    u32 output_pixel_size = width * height * 4;
    
    BMPHeader header = {0};
    header.file_type = 0x4D42;
    header.file_size = sizeof(header) + output_pixel_size;
    header.bitmap_offset = sizeof(header);
    header.size = sizeof(header) - 14;
    header.width = width;
    header.height = -height;
    header.planes = 1;
    header.bits_per_pixel = 32;
    header.compression = 0;
    header.size_of_bitmap = 0;
    header.horz_resolution = 0;
    header.vert_resolution = 0;
    header.colors_used = 0;
    header.colors_important = 0;
    
    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(&header, sizeof(header), 1, file);
        fwrite(data, output_pixel_size, 1, file);
        fclose(file);
    } else {
        fprintf(stderr, "[ERROR] Failed to open file '%s' for writing\n", filename);
        result = false;
    }
    
    return result;
}

typedef struct {
    u32 *pixels;
    u32 width;
    u32 height;
} Image;

static Image 
make_image_for_writing(u32 width, u32 height) {
    Image result;
    
    result.width = width;
    result.height = height;
    result.pixels = (u32 *)calloc(1, width * height * 4); 
    
    return result;
}

void 
image_save(Image *image, char *filename) {
    save_bmp(filename, image->width, image->height, image->pixels);
}

u32 *
image_get_pixel_pointer(Image *image, u32 x, u32 y) {
    u32 *result = image->pixels + y * image->width + x;
    return result;
}

typedef struct {
    Vec3 orig;
    Vec3 dir;
    Vec3 idir;
} Ray;

inline Ray 
make_ray(Vec3 orig, Vec3 dir) {
    Ray result;
    result.orig = orig;
    result.dir = dir;
    result.idir = vec3_neg(dir);
    return result;
}

inline void
ray_set_dir(Ray *ray, Vec3 dir) {
    ray->dir = dir;
    ray->idir = vec3_neg(dir);
}

inline Vec3 
ray_at(Ray ray, f32 t) {
    Vec3 result = vec3_add(ray.orig, vec3_muls(ray.dir, t));
    return result;
}

typedef struct {
    Vec3 orig;
    // Orthonormal basis
    Vec3 x;
    Vec3 y;
    Vec3 z;
    
    f32 focal_length;
    Vec3 film_center;
    
    f32 film_w;
    f32 film_h;
    f32 half_film_w;
    f32 half_film_h;
    
    f32 half_pix_w;
    f32 half_pix_h;
} Camera;

static Camera 
make_camera(Vec3 orig, Vec3 look_at, f32 film_width, f32 film_height, f32 focal_length) {
    Vec3 look_dir = normalize(vec3_sub(orig, look_at));
    
    Camera camera;
    
    camera.orig = orig;
    camera.z = look_dir;
    camera.x = normalize(cross(vec3(0, 0, 1), camera.z));
    camera.y = normalize(cross(camera.z, camera.x));
    
    camera.focal_length = focal_length;
    camera.film_center = vec3_muls(vec3_sub(camera.orig, camera.z), focal_length);
    
    camera.film_w = 1.0f;
    camera.film_h = 1.0f;
    if (film_width > film_height) {
        camera.film_h = (f32)film_height / (f32)film_width;
    } else if (film_height > film_width) {
        camera.film_w = (f32)film_width / (f32)film_height;
    }
    
    camera.half_film_w = camera.film_w * 0.5f;
    camera.half_film_h = camera.film_h * 0.5f;
    camera.half_pix_w = 1.0f / (f32)film_width * 0.5f;
    camera.half_pix_w = 1.0f / (f32)film_height * 0.5f;
    
    // @TODO lens aperture
    
    return camera;
}

static Ray 
camera_make_ray(Camera *camera, f32 u, f32 v) {
    assert(-1.0f <= u && u <= 1.0f && -1.0f <= v && v <= 1.0f);
    Vec3 film_pos = vec3_add(camera->film_center,
                             vec3_add(vec3_muls(camera->x, u * camera->half_film_w),
                                      vec3_muls(camera->y, v * camera->half_film_h)));
    
    Vec3 dir = normalize(vec3_sub(film_pos, camera->orig));
    Ray ray = make_ray(camera->orig, dir);
    
    return ray;
}

typedef struct {
    f32 t;
    Vec3 p;
    Vec3 n;
} HitRecord;

static bool 
ray_collide_sphere(Vec3 sphere_center, f32 sphere_radius, Ray ray, HitRecord *hit) {
    bool result = false;

    Vec3 rel_orig = vec3_sub(ray.orig, sphere_center);
    f32 a = length_sq(ray.dir);
    f32 half_b = dot(rel_orig, ray.dir);
    f32 c = length_sq(rel_orig) - sphere_radius * sphere_radius;
    
    f32 discriminant = half_b * half_b - a * c;
    
    if (discriminant > 0) {
        f32 root_term = sqrtf(discriminant);
        
        f32 tp = (-half_b + root_term) / a;
        f32 tn = (-half_b - root_term) / a;
        
        f32 t = tp;
        if ((tn > 0) && (tn < tp)) {
            t = tn;
        }
        
        if ((t > 0) && (t < hit->t)) {
            hit->t = t;
            hit->p = ray_at(ray, t);
            hit->n = vec3_divs(vec3_sub(hit->p, sphere_center), sphere_radius);
            
            result = true;
        }
    }
    
    return result;
}

typedef u32 ObjectType;
enum {
    ObjectType_Sphere  
};

typedef struct {
    ObjectType type;
    
    union {
        struct {
            Vec3 pos;  
            f32  radius; 
        } sphere;
    };
} Object;

typedef struct {
    Object objects[100];
    u32 object_count;
} World;

int 
main(int argc, char **argv) {
    u32 image_width = 1024;
    u32 image_height = 1024;
    
    World world = {0};
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .pos = vec3(0, 0, 0),
            .radius = 1.0f
        }
    };
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .pos = vec3(0, -100.5f, -1),
            .radius = 100.0f
        }
    };
    
    Image output_image = make_image_for_writing(image_width, image_height);
    Vec3 camera_orig = vec3(0, 2, 10);
    Vec3 camera_lookat = vec3(0, 0, 0);
    Camera camera = make_camera(camera_orig, camera_lookat, image_width, image_height, 1.0f);
    for (u32 y = 0;
         y < image_height;
         ++y) {
        f32 v = lerp(-1, 1, (f32)y / (f32)image_height);
        for (u32 x = 0;
             x < image_width;
             ++x) {
            f32 u = lerp(-1, 1, (f32)x / (f32)image_width);
            
            HitRecord hit = {0};
            hit.t = INFINITY;
            
            Ray ray = camera_make_ray(&camera, u, v);
            Vec3 color = vec3s(0.2f);
            
            for (u32 object_index = 0;
                 object_index < world.object_count;
                 ++object_index) {
                Object object = world.objects[object_index];
                
                switch(object.type) {
                    case ObjectType_Sphere: {
                        if (ray_collide_sphere(object.sphere.pos, object.sphere.radius, ray, &hit)) {
                            color = vec3_divs(vec3_add(hit.n, vec3s(1)), 2);
                        }
                    } break;
                }
            }
            
            u32 *pixel = image_get_pixel_pointer(&output_image, x, y);
            *pixel = rgba_pack_4x8_linear1(color.r, color.g, color.b, 1.0f);  
        }
    }
    image_save(&output_image, "out.bmp");
    
    
    
    printf("Exited successfully\n");
    return 0;
}
