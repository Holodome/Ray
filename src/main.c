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

#define PI 3.1415926535897932385f

#define DEG2RAD(_deg) ((_deg) * PI / 180.0f)

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
random_range(f32 low, f32 high) {
    f32 result = low + (high - low) * random_unitlateral();
    return result;
}

inline f32 
lerp(f32 a, f32 b, f32 t) {
    // assert(0 <= t && t <= 1);
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
v3neg(Vec3 a) {
    Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

inline Vec3 
v3add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

inline Vec3 
v3sub(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

inline Vec3 
v3div(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

inline Vec3 
v3mul(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

inline Vec3 
v3divs(Vec3 a, f32 b) {
    Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

inline Vec3 
v3muls(Vec3 a, f32 b) {
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
    Vec3 result = v3muls(a, rsqrtf(length_sq(a)));
    return result;
}

inline Vec3 
v3lerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    return result;
}

inline bool 
vec3_is_near_zero(Vec3 a) {
    const f32 epsilon = 1e-8;
    bool result = (fabsf(a.x) < epsilon) && (fabsf(a.y) < epsilon) && (fabsf(a.z) < epsilon);
    return result;
}

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
schlick(f32 cosine, f32 ref_idx)
{
    f32 r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    f32 result = r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
    return result;
}

inline Vec3 
random_unit_vector(void) {
    Vec3 result = vec3(random_bilateral(), random_bilateral(), random_bilateral());
    return result;
}

inline Vec3 
random_unit_sphere(void) {
    Vec3 result;
    do {
        result = random_unit_vector();
    } while(length_sq(result) >= 1.0f);
    return result;
}

inline Vec3 
random_unit_disk(void) {
    Vec3 result;
    do {
        result = vec3(random_bilateral(), random_bilateral(), 0);
    } while(length_sq(result) >= 1.0f);
    return result;
}

inline Vec3 
random_hemisphere(Vec3 normal) {
    Vec3 result = random_unit_sphere();
    if (dot(result, normal) > 0.0f) {
        
    } else {
        result = v3neg(result);
    }
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
save_bmp(char *filename, u32 width, u32 height, void *pixels) {
    bool result = true;
    
    u32 output_pixel_size = width * height * 4;
    
    BMPHeader header = {0};
    header.file_type = 0x4D42;
    header.file_size = sizeof(header) + output_pixel_size;
    header.bitmap_offset = sizeof(header);
    header.size = sizeof(header) - 14;
    header.width = width;
    header.height = height;
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
        fwrite(pixels, output_pixel_size, 1, file);
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
} Ray;

inline Ray 
make_ray(Vec3 orig, Vec3 dir) {
    Ray result;
    result.orig = orig;
    result.dir = dir;
    return result;
}

inline Vec3 
ray_at(Ray ray, f32 t) {
    Vec3 result = v3add(ray.orig, v3muls(ray.dir, t));
    return result;
}

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
    
    f32 film_w;
    f32 film_h;
} Camera;

static Camera 
make_camera(Vec3 look_from, Vec3 look_at, Vec3 v_up,
            f32 aspect_ratio, f32 vfov,
            f32 aperture, f32 focus_dist) {
    Camera camera;
    
    camera.orig = look_from;
    camera.z = normalize(v3sub(look_from, look_at));
    camera.x = normalize(cross(v_up, camera.z));
    camera.y = normalize(cross(camera.z, camera.x));
    
    f32 viewport_height = 2.0f * tanf(vfov * 0.5f);
    f32 viewport_width  = aspect_ratio * viewport_height;
    camera.horizontal = v3muls(camera.x, focus_dist * viewport_width);
    camera.vertical   = v3muls(camera.y, focus_dist * viewport_height);
    
    camera.lower_left_corner = camera.orig;
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.horizontal, 0.5f));
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.vertical, 0.5f));
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.z, focus_dist));   
    
    camera.lens_radius = aperture * 0.5f;
    
    return camera;
}

static Ray 
camera_make_ray(Camera *camera, f32 u, f32 v) {
    // u = u * 2 - 1;
    // v = v * 2 - 1;
    
    Vec3 rd = v3muls(random_unit_disk(), camera->lens_radius);
    Vec3 offset = v3add(v3muls(camera->x, rd.x), v3muls(camera->y, rd.y));
    
    Vec3 dir = camera->lower_left_corner;
    dir = v3add(dir, v3muls(camera->horizontal, u));
    dir = v3add(dir, v3muls(camera->vertical, v));
    dir = v3sub(dir, camera->orig);
    dir = v3sub(dir, offset);
    dir = normalize(dir);
    
    Vec3 orig = v3add(camera->orig, offset);
    Ray ray = make_ray(orig, dir);
    
    return ray;
}

typedef struct {
    bool has_hit;
    f32 t;
    Vec3 p;
    Vec3 n;
    bool is_front_face;
    
    u32 mat_index;
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

static bool 
ray_collide_sphere(Vec3 sphere_center, f32 sphere_radius, Ray ray, HitRecord *hit) {
    bool result = false;

    Vec3 rel_orig = v3sub(ray.orig, sphere_center);
    f32 a = length_sq(ray.dir);
    f32 half_b = dot(rel_orig, ray.dir);
    f32 c = length_sq(rel_orig) - sphere_radius * sphere_radius;
    
    f32 discriminant = half_b * half_b - a * c;
    
    if (discriminant >= 0) {
        f32 root_term = sqrtf(discriminant);
        
        f32 tp = (-half_b + root_term) / a;
        f32 tn = (-half_b - root_term) / a;
        
        f32 t = tp;
        if ((tn > 0.001f) && (tn < tp)) {
            t = tn;
        }
        
        if ((t > 0.001f) && (t < hit->t)) {
            hit->t = t;
            hit->p = ray_at(ray, t);
            hit_set_normal(hit, v3divs(v3sub(hit->p, sphere_center), sphere_radius), ray);
            
            result = true;
        }
    }
    
    return result;
}

typedef u32 MaterialType;
enum {
    MaterialType_None = 0x0,
    MaterialType_Metal,
    MaterialType_Lambertian,
    MaterialType_Dielectric,
};

typedef struct {
    MaterialType type;
    
    union {
        struct {
            Vec3 albedo;
            f32 fuzz; 
        } metal;
        struct {
            Vec3 albedo;
        } lambertian;
        struct {
            f32 ir;
        } dielectric;
    };
} Material;

typedef u32 ObjectType;
enum {
    ObjectType_None = 0x0,
    ObjectType_Sphere  
};

typedef struct {
    ObjectType type;
    
    u32 mat_index;
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
    
    Material materials[100];
    u32 material_count;
} World;

int 
main(int argc, char **argv) {
    u32 image_width = 720;
    // u32 image_height = 720;
    u32 image_height = 405;
    
    World world = {0};
    world.materials[world.material_count++] = (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = vec3(0.8f, 0.8f, 0.0f)
        }  
    };
    world.materials[world.material_count++] = (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = vec3(0.1f, 0.2f, 0.5f)
        }  
    };
    world.materials[world.material_count++] = (Material) {
        .type = MaterialType_Dielectric,
        .dielectric = {
            .ir = 1.5f
        }
    };
    world.materials[world.material_count++] = (Material) {
        .type = MaterialType_Metal,
        .metal = {
            .albedo = vec3(0.8, 0.6, 0.2),
            .fuzz = 0.0f
        }  
    };
    
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere, 
        .mat_index = 1, 
        .sphere = {
            .pos = vec3(0, 0, -1),
            .radius = 0.5f
        }
    };
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere,  
        .mat_index = 0,
        .sphere = {
            .pos = vec3(0, -100.5f, -1),
            .radius = 100.0f
        }
    };
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere,  
        .mat_index = 3,
        .sphere = {
            .pos = vec3(1, 0, -1),
            .radius = 0.5f
        }
    };
    // world.objects[world.object_count++] = (Object) {
    //     .type = ObjectType_Sphere,  
    //     .mat_index = 2,
    //     .sphere = {
    //         .pos = vec3(-1, 0, -1),
    //         .radius = -0.45f
    //     }
    // };
    world.objects[world.object_count++] = (Object) {
        .type = ObjectType_Sphere,  
        .mat_index = 2,
        .sphere = {
            .pos = vec3(-1, 0, -1),
            .radius = 0.5f
        }
    };
    
    u32 samples_per_pixel = 32;
    u32 max_bounce_count = 8;
    
    f32 aspect_ratio = (f32)image_width / (f32)image_height;
    Image output_image = make_image_for_writing(image_width, image_height);
    Vec3 look_from = vec3(3, 1, 2);
    Vec3 look_at = vec3(0, 0, -1);
    Camera camera = make_camera(look_from, look_at, vec3(0, 1, 0), aspect_ratio, DEG2RAD(20), 0.0f, length(v3sub(look_from, look_at)));
    for (u32 y = 0;
         y < image_height;
         ++y) {
        for (u32 x = 0;
             x < image_width;
             ++x) {
            Vec3 pixel_color = vec3(0, 0, 0);
            
            for (u32 sample_index = 0;
                sample_index < samples_per_pixel;
                ++sample_index) {
                
                f32 u = ((f32)x + random_bilateral() * 0.5f) / (f32)image_width;
                f32 v = ((f32)y + random_bilateral() * 0.5f) / (f32)image_height;
                Ray ray = camera_make_ray(&camera, u, v);
                Vec3 attenuation = vec3(1, 1, 1);
                Vec3 sample_color = vec3(0, 0, 0);
                
                for (u32 bounce_count = 0;
                    bounce_count < max_bounce_count;
                    ++bounce_count) {
                    
                    HitRecord hit = {0};
                    hit.t = INFINITY;
                    
                    Vec3 color = vec3(0, 0, 0);
                    
                    for (u32 object_index = 0;
                        object_index < world.object_count;
                        ++object_index) {
                        Object object = world.objects[object_index];
                        
                        switch(object.type) {
                            case ObjectType_Sphere: {
                                if (ray_collide_sphere(object.sphere.pos, object.sphere.radius, ray, &hit)) {
                                    hit.mat_index = object.mat_index;
                                }
                            } break;
                            default: assert(false);
                        }
                    }
                    
                    if (hit.t == INFINITY) {
                        color = vec3(0.5f, 0.7f, 1.0f);
                        sample_color = v3add(sample_color, v3mul(color, attenuation));
                        
                        break;
                    } else {
                        Material material = world.materials[hit.mat_index];
                        switch (material.type) {
                            case MaterialType_Lambertian: {
                                Vec3 scatter_dir = normalize(v3add(hit.n, random_unit_vector()));
                                if (vec3_is_near_zero(scatter_dir)) {
                                    scatter_dir = hit.n;
                                }
                                ray.dir = scatter_dir;
                                attenuation = v3mul(attenuation, material.lambertian.albedo);
                            } break;
                            case MaterialType_Metal: {
                                Vec3 reflected = reflect(ray.dir, hit.n);
                                Vec3 random = random_unit_sphere();
                                Vec3 scattered = normalize(v3add(reflected, v3muls(random, material.metal.fuzz)));
                                if (dot(scattered, hit.n) > 0.0f) {
                                    ray.dir = scattered;
                                    attenuation = v3mul(attenuation, material.metal.albedo);
                                }
                            } break;
                            case MaterialType_Dielectric: {
                                f32 refraction_ratio = material.dielectric.ir;
                                if (hit.is_front_face) {
                                    refraction_ratio = 1.0f / refraction_ratio;
                                }
                                    
                                f32 cos_theta = fminf(dot(v3neg(ray.dir), hit.n), 1.0f);
                                f32 sin_theta = sqrtf(1.0f - cos_theta * cos_theta);
                                
                                Vec3 scattered;
                                if ((refraction_ratio * sin_theta > 1.0f) || schlick(cos_theta, refraction_ratio) > random_unitlateral()) {
                                    scattered = reflect(ray.dir, hit.n);
                                } else {
                                    scattered = refract(ray.dir, hit.n, refraction_ratio);
                                }
                                ray.dir = normalize(scattered);
                            } break;
                            default: assert(false);
                        }

                        ray.orig = hit.p;
                    }
                }
                
                f32 color_multiplier = 1.0f / (f32)samples_per_pixel;
                pixel_color = v3add(pixel_color, v3muls(sample_color, color_multiplier));
            }

            
            u32 *pixel = image_get_pixel_pointer(&output_image, x, y);
            *pixel = rgba_pack_4x8_linear1(sqrtf(pixel_color.b), sqrtf(pixel_color.g), sqrtf(pixel_color.r), 1.0f);  
        }
    }
    image_save(&output_image, "out.bmp");
    
    printf("Exited successfully\n");
    return 0;
}
