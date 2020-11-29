#if !defined(TRACE_H)

#include "general.h"
#include "image.h"
#include "perlin.h"

typedef struct { u32 v; } TextureHandle;
typedef struct { u32 v; } MaterialHandle;
typedef struct { u32 v; } ObjectHandle;

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

//
// Ray
//

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

//
// Camera
//

typedef struct {
    f32 aspect_ratio;
    
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
camera_make_ray(Camera *camera, RandomSeries *entropy, f32 u, f32 v) {
    Vec3 rd = v3muls(random_unit_disk(entropy), camera->lens_radius);
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

static void 
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

typedef u32 ObjectType;
enum {
    ObjectType_None = 0x0,
    ObjectType_ObjectList,
    ObjectType_Sphere,
    ObjectType_Triangle,
    ObjectType_ConstantMedium
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
            u32 objects_size;
            u32 objects_capacity;
        } object_list;
    };
} Object;

typedef struct {
    Camera camera;
    
    Vec3 backgorund_color;
    
    Texture *textures;
    u32 textures_size;
    u32 textures_capacity;

    Material *materials;
    u32 materials_size;
    u32 materials_capacity;
    
    Object *objects;
    u32 objects_size;
    u32 objects_capacity;
    
    ObjectHandle object_list;
} World;

// Stupid dynamic array hack
#define IAMLAZYTOINITWORLD(_arr, _capacity) { if (!_capacity) { _capacity = 10; _arr = malloc(sizeof(*_arr) * _capacity); } }
#define EXPAND_IF_NEEDED(_arr, _size, _capacity) { IAMLAZYTOINITWORLD(_arr, _capacity); if (_size + 1 > _capacity) { _capacity *= 2; _arr = realloc(_arr, sizeof(*_arr) * _capacity); } }

TextureHandle  
add_texture(World *world, Texture texture) {
    EXPAND_IF_NEEDED(world->textures, world->textures_size, world->textures_capacity);
    
    TextureHandle handle = { world->textures_size };
    world->textures[world->textures_size++] = texture;
    
    return handle;
}

MaterialHandle  
add_material(World *world, Material material) {
    EXPAND_IF_NEEDED(world->materials, world->materials_size, world->materials_capacity);
    
    MaterialHandle handle = { world->materials_size };
    world->materials[world->materials_size++] = material;
    
    return handle;
}

ObjectHandle 
add_object(World *world, Object object) {
    EXPAND_IF_NEEDED(world->objects, world->objects_size, world->objects_capacity);
    
    ObjectHandle handle = { world->objects_size };
    world->objects[world->objects_size++] = object;
    
    return handle;
}

void  
add_object_to_list(World *world, ObjectHandle list_handle, ObjectHandle object) {
    Object *list = world->objects + list_handle.v;
    
    EXPAND_IF_NEEDED(list->object_list.objects, list->object_list.objects_size, list->object_list.objects_capacity);
    assert(list->type = ObjectType_ObjectList);
    
    list->object_list.objects[list->object_list.objects_size++] = object;
}

Vec3
sample_texture(World *world, TextureHandle handle, HitRecord *hit) {
    Vec3 result = {0};
    
    Texture *texture = world->textures + handle.v; 
    switch(texture->type) {
        case TextureType_Solid: {
            result = texture->solid.c;
        } break;
        case TextureType_Checkered: {
            f32 sines = sinf(10 * hit->p.x) * sinf(10 * hit->p.y) * sinf(10 * hit->p.z);
            if (sines < 0) {
                result = sample_texture(world, texture->checkered.t1, hit);
            } else {
                result = sample_texture(world, texture->checkered.t2, hit);
            }
        } break;
        case TextureType_Image: {
            f32 u = clamp(hit->u, 0, 1);
            f32 v = clamp(hit->v, 0, 1);
            
            u32 x = roundf(u * texture->image.i.w);
            u32 y = roundf(v * texture->image.i.h);
            
            if (x >= texture->image.i.w) {
                x = texture->image.i.w - 1;
            }
            if (y >= texture->image.i.h) {
                y = texture->image.i.h - 1;
            }
            
            f32 color_scale = 1.0f / 255.0f;
            u8 *pixel = (u8 *)image_get_pixel_pointer(&texture->image.i, x, y);
            
            result = v3(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
        } break;
        case TextureType_Perlin: {
            // f32 noise = perlin_turb(&texture->perlin.p, v3muls(hit->p, texture->perlin.s), 7);
            f32 noise = 0.5f * (1 + sinf(texture->perlin.s * hit->p.z + 10 * perlin_turb(&texture->perlin.p, hit->p, 7)));
            result = v3muls(v3s(1), noise);
        } break;
        case TextureType_UV: {
            result = v3(hit->u, hit->v, 0);
        } break;
        case TextureType_Normal: {
            result = v3muls(v3add(hit->n, v3s(1)), 0.5f);
        } break;
        default: assert(false);
    }
    return result;
} 

inline Texture
texture_solid(Vec3 c) {
    return (Texture) {
        .type = TextureType_Solid,
        .solid = {
            .c = c
        }
    };
}

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

inline Object 
object_sphere(Vec3 p, f32 r, MaterialHandle mat) {
    return (Object) {
        .type = ObjectType_Sphere,
        .sphere = {
            .mat = mat,
            .p = p,
            .r = r
        }
    };
}

inline Object 
object_triangle(Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat) {
    Vec3 n = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
    return (Object) {
        .type = ObjectType_Triangle,
        .triangle = {
            .mat = mat,
            .p[0] = p0,
            .p[1] = p1,
            .p[2] = p2,
            .n = n
        }
    };
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
} RayCastData;

static bool 
object_hit(World *world, RandomSeries *rs, ObjectHandle obj_handle, Ray ray, HitRecord *hit,
           f32 t_min, f32 t_max) {
    bool result = false;
    
    Object object = world->objects[obj_handle.v];
    switch(object.type) {
        case ObjectType_Sphere: {
            Vec3 rel_orig = v3sub(ray.orig, object.sphere.p);
            f32 a = length_sq(ray.dir);
            f32 half_b = dot(rel_orig, ray.dir);
            f32 c = length_sq(rel_orig) - object.sphere.r * object.sphere.r;
            
            f32 discriminant = half_b * half_b - a * c;
            
            if (discriminant >= 0) {
                f32 root_term = sqrtf(discriminant);
                
                f32 tp = (-half_b + root_term) / a;
                f32 tn = (-half_b - root_term) / a;
                
                f32 t = tp;
                if ((tn > t_min) && (tn < tp)) {
                    t = tn;
                }
                
                if ((t > t_min) && (t < t_max)) {
                    hit->t = t;
                    hit->p = ray_at(ray, hit->t);
                    Vec3 outward_normal = v3divs(v3sub(hit->p, object.sphere.p), object.sphere.r);
                    hit_set_normal(hit, outward_normal, ray);
                    f32 u, v;
                    sphere_get_uv(outward_normal, &u, &v);
                    hit->u = u;
                    hit->v = v;
                    
                    hit->mat = object.sphere.mat;
                    result = true;
                }
            }
        } break;
        case ObjectType_Triangle: {
            Vec3 e1 = v3sub(object.triangle.p[1], object.triangle.p[0]);
            Vec3 e2 = v3sub(object.triangle.p[2], object.triangle.p[0]);
            Vec3 h = cross(ray.dir, e2);
            f32 a = dot(e1, h);
            
            if ((a < 0.001f) || (a > 0.001f)) {
                f32 f = 1.0f / a;
                Vec3 s = v3sub(ray.orig, object.triangle.p[0]);
                f32 u = f * dot(s, h);
                Vec3 q = cross(s, e1);
                f32 v = f * dot(ray.dir, q);
                f32 t = f * dot(e2, q);
                if ((0 < u) && (u < 1) && (v > 0) && (u + v < 1)) {
                    if ((t > t_min) && (t < t_max)) {
                        hit->t = t;
                        hit->p = ray_at(ray, hit->t);
                        // Vec3 outward_normal = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
                        Vec3 outward_normal = object.triangle.n;
                        hit_set_normal(hit, outward_normal, ray);
                        // @NOTE these are not actual uvs
                        hit->u = u;
                        hit->v = v;
                        
                        hit->mat = object.triangle.mat;
                        result = true;
                    }
                }
            }
        } break;
        case ObjectType_ObjectList: {
            bool has_hit_anything = false;
            
            f32 closest_so_far = t_max;
            for (u32 object_index = 0;
                object_index < object.object_list.objects_size;
                ++object_index) {
                ObjectHandle test_object = object.object_list.objects[object_index];
              
                HitRecord temp_hit = {0};  
                if (object_hit(world, rs, test_object, ray, &temp_hit, t_min, closest_so_far)) {
                    has_hit_anything = true;
                    closest_so_far = temp_hit.t;
                    *hit = temp_hit;
                }
            }
            
            result = has_hit_anything;
        } break;
        case ObjectType_ConstantMedium: {
            HitRecord hit1 = {0}, hit2 = {0};
            if (object_hit(world, rs, object.constant_medium.boundary, ray, &hit1, -INFINITY, INFINITY)) {
                if (object_hit(world, rs, object.constant_medium.boundary, ray, &hit2, hit1.t + 0.0001f, INFINITY)) {
                    if (hit1.t < t_min) {
                        hit1.t = t_min;
                    }
                    if (hit2.t > t_max) {
                        hit2.t = t_max;
                    }
                    
                    if (hit1.t < hit2.t) {
                        if (hit1.t < 0) {
                            hit1.t = 0;
                        }
                        
                        f32 distance_inside_boundary = hit2.t - hit1.t;
                        f32 hit_dist = object.constant_medium.neg_inv_density * logf(random(rs));
                        
                        if (hit_dist < distance_inside_boundary) {
                            hit->t = hit1.t + hit_dist;
                            hit->p = ray_at(ray, hit->t);
                            
                            hit->n = v3(1, 0, 0);
                            hit->is_front_face = true;
                            hit->mat = object.constant_medium.phase_function;
                            
                            result = true;
                        } 
                    }
                }
            }
        } break;
        default: assert(false);
    }
    
    return result;
}

static Vec3 
ray_cast(World *world, RandomSeries *entropy, Ray ray,
         RayCastData *data) {
    Vec3 attenuation = v3(1, 1, 1);
    Vec3 sample_color = v3(0, 0, 0);
    
    for (u32 bounce_count = 0;
        bounce_count < MAX_BOUNCE_COUNT;
        ++bounce_count) {
        ++data->bounce_count;
        
        HitRecord hit = {0};
        hit.t = INFINITY;
        
        Vec3 color = v3(0, 0, 0);
        
        if (!object_hit(world, entropy, world->object_list, ray, &hit, 0.001f, INFINITY)) {
            // f32 t = 0.5f * (ray.dir.y + 1.0f); 
            // color = v3lerp(v3s(1.0f), v3(0.5f, 0.7f, 1.0f), t);
            color = world->backgorund_color;
            sample_color = v3add(sample_color, v3mul(color, attenuation));
            
            goto finished_casting;
        } else {
            Material material = world->materials[hit.mat.v];
            switch (material.type) {
                case MaterialType_Lambertian: {
                    Vec3 scatter_dir = v3add(hit.n, random_unit_vector(entropy));
                    if (vec3_is_near_zero(scatter_dir)) {
                        scatter_dir = hit.n;
                    }
                    ray.dir = normalize(scatter_dir);
                    
                    Vec3 sampled_texture = sample_texture(world, material.lambertian.albedo, &hit);
                    attenuation = v3mul(attenuation, sampled_texture);
                } break;
                case MaterialType_Metal: {
                    Vec3 reflected = reflect(ray.dir, hit.n);
                    Vec3 random = random_unit_sphere(entropy);
                    Vec3 scattered = normalize(v3add(reflected, v3muls(random, material.metal.fuzz)));
                    if (dot(scattered, hit.n) > 0.0f) {
                        ray.dir = scattered;

                        Vec3 sampled_texture = sample_texture(world, material.metal.albedo, &hit);
                        attenuation = v3mul(attenuation, sampled_texture);
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
                    if ((refraction_ratio * sin_theta > 1.0f) || schlick(cos_theta, refraction_ratio) > random(entropy)) {
                        scattered = reflect(ray.dir, hit.n);
                    } else {
                        scattered = refract(ray.dir, hit.n, refraction_ratio);
                    }
                    ray.dir = normalize(scattered);
                } break;
                case MaterialType_DiffuseLight: {
                    Vec3 emit_color = sample_texture(world, material.diffuse_light.emit, &hit);
                    sample_color = v3add(sample_color, v3mul(attenuation, emit_color));

                    goto finished_casting;
                } break;
                case MaterialType_Isotropic: {
                    ray.dir = random_unit_sphere(entropy);
                    attenuation = v3mul(attenuation, sample_texture(world, material.isotropic.albedo, &hit));
                } break;
                default: assert(false);
            }

            ray.orig = hit.p;
        }
    }
    
finished_casting: 
    
    return sample_color;
}


#define TRACE_H 1
#endif
