#include "trace.h"

void 
add_xy_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 y0, f32 y1, f32 z, MaterialHandle mat) {
    Vec3 v00 = v3(x0, y0, z);
    Vec3 v01 = v3(x0, y1, z);
    Vec3 v10 = v3(x1, y0, z);
    Vec3 v11 = v3(x1, y1, z);
    
    add_object(world, list, object_triangle(world, v00, v01, v11, mat));
    add_object(world, list, object_triangle(world, v00, v11, v10, mat));
}

void 
add_yz_rect(World *world, ObjectHandle list, f32 y0, f32 y1, f32 z0, f32 z1, f32 x, MaterialHandle mat) {
    Vec3 v00 = v3(x, y0, z0);
    Vec3 v01 = v3(x, y0, z1);
    Vec3 v10 = v3(x, y1, z0);
    Vec3 v11 = v3(x, y1, z1);
    
    add_object(world, list, object_triangle(world,  v00, v01, v11, mat));
    add_object(world, list, object_triangle(world,  v00, v11, v10, mat));
}

void 
add_xz_rect(World *world, ObjectHandle list, f32 x0, f32 x1, f32 z0, f32 z1, f32 y, MaterialHandle mat) {
    Vec3 v00 = v3(x0, y, z0);
    Vec3 v01 = v3(x0, y, z1);
    Vec3 v10 = v3(x1, y, z0);
    Vec3 v11 = v3(x1, y, z1);
    
    add_object(world, list, object_triangle(world, v00, v01, v11, mat));
    add_object(world, list, object_triangle(world, v00, v11, v10, mat));
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

static bool 
bounds3_hit(Bounds3 bounds, Ray ray, f32 t_min, f32 t_max) {
    for (u32 a = 0;
         a < 3;
         ++a) {
        f32 inv_d = 1.0f / ray.dir.e[a];
        f32 t0 = (bounds.min.e[a] - ray.orig.e[a]) * inv_d;
        f32 t1 = (bounds.max.e[a] - ray.orig.e[a]) * inv_d;
        if (inv_d < 0.0f) {
            f32 temp = t0;
            t0 = t1;
            t1 = temp;
        }
        
        if (t0 > t_min) {
            t_min = t0;
        }
        if (t1 < t_max) {
            t_max = t1;
        }
        
        if (t_max < t_min) {
            return false;
        }
    }
    return true;
}

#define BOX_COMPARATOR_SIGNATURE(_name) int _name(void *w, const void *a, const void *b)
typedef BOX_COMPARATOR_SIGNATURE(BoxComparator);

static int 
box_comapre(World *world, ObjectHandle a, ObjectHandle b, u32 axis) {
    Bounds3 b0, b1;
    bool has_left_bounds = get_object_bounds(world, a, &b0);
    bool has_right_bounds = get_object_bounds(world, b, &b1);
    assert(has_left_bounds && has_right_bounds);
    return b0.min.e[axis] < b1.min.e[axis];
}

static BOX_COMPARATOR_SIGNATURE(box_compare_x) { return box_comapre((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 0); }
static BOX_COMPARATOR_SIGNATURE(box_compare_y) { return box_comapre((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 1); }
static BOX_COMPARATOR_SIGNATURE(box_compare_z) { return box_comapre((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 2); }

static Vec3 
reflect(Vec3 v, Vec3 n) {
    Vec3 result = v3sub(v, v3muls(n, 2.0f * dot(v, n)));
    return result;
}

static Vec3 
refract(Vec3 v, Vec3 n, f32 etai_over_etat) {
    f32 cos_theta = min32(-dot(v, n), 1.0f);
    Vec3 r_out_perp = v3muls(v3add(v, v3muls(n, cos_theta)), etai_over_etat);
    Vec3 r_out_parallel = v3muls(n, -sqrt32(abs32(1.0f - length_sq(r_out_perp))));
    Vec3 result = v3add(r_out_perp, r_out_parallel);
    return result; 
}

static f32
schlick(f32 cosine, f32 ref_idx) {
    f32 r0 = (1.0f - ref_idx) / (1.0f + ref_idx);
    r0 = r0 * r0;
    f32 result = r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
    return result;
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

Ray 
make_ray(Vec3 orig, Vec3 dir) {
    return (Ray) {
        .orig = orig,
        .dir = dir
    };
}

Vec3 
ray_at(Ray ray, f32 t) {
    Vec3 result = v3add(ray.orig, v3muls(ray.dir, t));
    return result;
}

void 
hit_set_normal(HitRecord *hit, Vec3 n, Ray ray) {
    if (dot(ray.dir, n) < 0.0f) {
        hit->is_front_face = true;
        hit->n = n;
    } else {
        hit->is_front_face = false;
        hit->n = v3neg(n);
    }
}

Camera 
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

Ray 
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

// Dynamic array hacks
#define DEFAULT_ARRAY_CAPACITY 10
#define IAMLAZYTOINITWORLD(_a, _arr, _capacity) { if (!_capacity) { _capacity = DEFAULT_ARRAY_CAPACITY; _arr = arena_alloc(_a, sizeof(*_arr) * _capacity); } }
#define EXPAND_IF_NEEDED(_a, _arr, _size, _capacity) { IAMLAZYTOINITWORLD(_a, _arr, _capacity);  \
    if (_size + 1 > _capacity) { _arr = arena_realloc(_a, _arr, sizeof(*_arr) * _capacity, sizeof(*_arr) * _capacity * 2); _capacity *= 2; } }
#define SHRINK_TO_FIT(_a, _arr, _size, _capacity) { _arr = arena_realloc(_a, _arr, sizeof(*_arr) * _capacity, sizeof(*_arr) * _size); _capacity = _size; }

ObjectList 
object_list_init(MemoryArena *arena, u32 reserve) {
    ObjectList lst = {0};
    lst.capacity = reserve;
    lst.a = arena_alloc(arena, lst.capacity * sizeof(*lst.a));
    
    return lst;
}

void
add_object_to_list(MemoryArena *arena, ObjectList *list, ObjectHandle o) {
    EXPAND_IF_NEEDED(arena, list->a, list->size, list->capacity);
    
    list->a[list->size++] = o;   
}

void
object_list_shrink_to_fit(MemoryArena *arena, ObjectList *list) {
    SHRINK_TO_FIT(arena, list->a, list->size, list->capacity);
}


PDF
cosine_pdf(Vec3 w) {
    return (PDF) {
        .type = PDFType_Cosine,
        .cosine = {
            .uvw = onb_from_w(w)
        }
    };
}

PDF
object_pdf(ObjectHandle obj, Vec3 orig) {
    return (PDF) {
        .type = PDFType_Object,
        .object = {
            .o = orig,
            .object = obj
        }
    };
}

TextureHandle 
new_texture(World *world, Texture texture) {
    EXPAND_IF_NEEDED(&world->arena, world->textures, world->textures_size, world->textures_capacity);
    
    TextureHandle handle = { world->textures_size };
    world->textures[world->textures_size++] = texture;
    
    return handle;
}

MaterialHandle 
new_material(World *world, Material material) {
    EXPAND_IF_NEEDED(&world->arena, world->materials, world->materials_size, world->materials_capacity);
    
    MaterialHandle handle = { world->materials_size };
    world->materials[world->materials_size++] = material;
    
    return handle;
}

ObjectHandle 
new_object(World *world, Object object) {
    EXPAND_IF_NEEDED(&world->arena, world->objects, world->objects_size, world->objects_capacity);
    
    ObjectHandle handle = { world->objects_size };
    world->objects[world->objects_size++] = object;
    
    return handle;
}

Texture *
get_texture(World *world, TextureHandle h) {
    return world->textures + h.v;
}

Material *
get_material(World *world, MaterialHandle h) {
    return world->materials + h.v;
}

Object *
get_object(World *world, ObjectHandle h) {
    return world->objects + h.v;    
}

void  
add_object(World *world, ObjectHandle list_handle, ObjectHandle object) {
    Object *list = get_object(world, list_handle);
    assert(list->type = ObjectType_ObjectList);
 
    add_object_to_list(&world->arena, &list->object_list, object);   
}

void 
add_object_to_world(World *world, ObjectHandle o) {
    add_object(world, world->object_list, o);
}

TextureHandle 
texture_solid(World *world, Vec3 c) {
    Texture texture;
    texture.type = TextureType_Solid;
    texture.solid.c = c;
    
    return new_texture(world, texture);    
}

TextureHandle 
texture_checkered(World *world, TextureHandle t1, TextureHandle t2) {
    Texture texture;
    texture.type = TextureType_Checkered;
    texture.checkered.t1 = t1;    
    texture.checkered.t2 = t2;    
    
    return new_texture(world, texture);
}

TextureHandle 
texture_image(World *world, Image i) {
    Texture texture;
    texture.type = TextureType_Image;
    texture.image.i = i;
    
    return new_texture(world, texture);
}

TextureHandle 
texture_perlin(World *world, Perlin perlin, f32 s) {
    Texture texture;
    texture.type = TextureType_Perlin;
    texture.perlin.p = perlin;
    texture.perlin.s = s;
    
    return new_texture(world, texture);
}

TextureHandle 
texture_uv(World *world) {
    Texture texture;
    texture.type = TextureType_UV;
    
    return new_texture(world, texture); 
}

TextureHandle 
texture_normal(World *world) {
    Texture texture;
    texture.type = TextureType_Normal;
    
    return new_texture(world, texture);
}

MaterialHandle 
material_lambertian(World *world, TextureHandle albedo) {
    Material material;
    material.type = MaterialType_Lambertian;
    material.lambertian.albedo = albedo;
    
    return new_material(world, material);    
}

MaterialHandle 
material_isotropic(World *world, TextureHandle albedo) {
    Material material;
    material.type = MaterialType_Isotropic;
    material.isotropic.albedo = albedo;
    
    return new_material(world, material);    
}

MaterialHandle 
material_metal(World *world, TextureHandle albedo, f32 fuzz) {
    Material material;
    material.type = MaterialType_Metal;
    material.metal.albedo = albedo;
    material.metal.fuzz = fuzz;
    
    return new_material(world, material);    
}

MaterialHandle 
material_dielectric(World *world, f32 ir) {
    Material material;
    material.type = MaterialType_Dielectric;
    material.dielectric.ir = ir;
    
    return new_material(world, material);    
}

MaterialHandle 
material_diffuse_light(World *world, TextureHandle t) {
    Material material;
    material.type = MaterialType_DiffuseLight;
    material.diffuse_light.emit = t;
    
    return new_material(world, material);    
}

ObjectHandle 
object_list(World *world) {
    Object object;
    object.type = ObjectType_ObjectList;
    object.object_list = object_list_init(&world->arena, 0);
    
    return new_object(world, object);        
}

ObjectHandle 
object_flip_face(World *world, ObjectHandle obj) {
    Object object;
    object.type = ObjectType_FlipFace;
    object.flip_face.obj = obj;
    
    return new_object(world, object);        
}

ObjectHandle 
object_sphere(World *world, Vec3 p, f32 r, MaterialHandle mat) {
    Object object;
    object.type = ObjectType_Sphere;
    object.sphere.mat = mat;
    object.sphere.p = p;
    object.sphere.r = r;
    
    return new_object(world, object);        
}

ObjectHandle 
object_instance(World *world, ObjectHandle obj, Vec3 t, Vec3 r) {
    Mat4x4 o2w = MAT4X4_IDENTITIY;
    o2w = mat4x4_mul(o2w, mat4x4_translate(t));
    o2w = mat4x4_mul(o2w, mat4x4_rotation(r.x, v3(1, 0, 0)));
    o2w = mat4x4_mul(o2w, mat4x4_rotation(r.y, v3(0, 1, 0)));
    o2w = mat4x4_mul(o2w, mat4x4_rotation(r.z, v3(0, 0, 1)));
    
    Bounds3 ob;
    bool has_bounds = get_object_bounds(world, obj, &ob);
    assert(has_bounds);
    
    Bounds3 bounds = bounds3i(mat4x4_mul_vec3(o2w, ob.min));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.max.x, ob.min.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.min.x, ob.max.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.min.x, ob.min.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.min.x, ob.max.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.max.x, ob.max.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.max.x, ob.min.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(o2w, v3(ob.max.x, ob.max.y, ob.max.z)));
    
    Object object;
    object.type = ObjectType_Instance;
    object.instance.bounds = bounds;
    object.instance.o2w = o2w;
    object.instance.w2o = mat4x4_inverse(o2w);
    object.instance.object = obj;
    
    return new_object(world, object);        
}

ObjectHandle 
object_triangle(World *world, Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat) {
    Vec3 n = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
    
    Object object;
    object.type = ObjectType_Triangle;
    object.triangle.p[0] = p0;
    object.triangle.p[1] = p1;
    object.triangle.p[2] = p2;
    object.triangle.n = n;
    object.triangle.mat = mat;
    
    return new_object(world, object);        
}

ObjectHandle 
object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat) {
    Object object;
    object.type = ObjectType_Box;
    object.box.bounds = bounds3(min, max);
    object.box.sides_list = object_list(world);
    add_box(world, object.box.sides_list, min, max, mat);
    object_list_shrink_to_fit(&world->arena, &get_object(world, object.box.sides_list)->object_list);
    
    return new_object(world, object);        
}

ObjectHandle 
object_constant_medium(World *world, f32 d, MaterialHandle phase, ObjectHandle bound) {
    Object object;
    object.type = ObjectType_ConstantMedium;
    object.constant_medium.boundary = bound;
    object.constant_medium.neg_inv_density = -1.0f / d;
    object.constant_medium.phase_function = phase;
    
    return new_object(world, object);        
}

ObjectHandle 
object_bvh_node(World *world, ObjectList object_list, u64 start, u64 end) {
    Object object;
    object.type = ObjectType_BVHNode;
    
    u32 axis = random_int(&global_entropy, 0, 2);
    // BoxComparator *cs[] = { box_compare_x, box_compare_y, box_compare_z };
    // BoxComparator *comparator = cs[axis];
#define COMPARATOR (axis == 0 ? box_compare_x : axis == 1 ? box_compare_y : box_compare_z)
                                            
    assert(end > start);
    u64 object_count = end - start;
    
    TempMemory temp_mem = temp_memory_begin(&world->arena);
    ObjectHandle *objects = arena_alloc(&world->arena, sizeof(ObjectHandle) * object_list.size);
    memcpy(objects, object_list.a, sizeof(ObjectHandle) * object_list.size);
    
    if (object_count == 1) {
        object.bvh_node.left = object.bvh_node.right = objects[start];    
    } else if (object_count == 2) {
        if (COMPARATOR(world, objects + start, objects + start + 1)) {
            object.bvh_node.left = objects[start];
            object.bvh_node.right = objects[start + 1];
        } else {
            object.bvh_node.left = objects[start + 1];
            object.bvh_node.right = objects[start];
        }
    } else {
        qsort_s(objects + start, object_count, sizeof(ObjectHandle), COMPARATOR, world);
        
        u64 mid = start + object_count / 2;
        
        object.bvh_node.left = object_bvh_node(world, object_list, start, mid);
        object.bvh_node.right = object_bvh_node(world, object_list, mid, end);
    }
    temp_memory_end(temp_mem);
    
    Bounds3 bounds_left, bounds_right;
    bool has_left_bounds = get_object_bounds(world, object.bvh_node.left, &bounds_left);
    bool has_right_bounds = get_object_bounds(world, object.bvh_node.right, &bounds_right);
    assert(has_left_bounds && has_right_bounds);
    object.bvh_node.bounds = bounds3_join(bounds_left, bounds_right);
    
    return new_object(world, object);        
}

Vec3
sample_texture(World *world, TextureHandle handle, HitRecord *hit) {
    Vec3 result;
    
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
        INVALID_DEFAULT_CASE;
    }
    return result;
} 

bool  
material_scatter(World *world, RandomSeries *entropy, MaterialHandle mat, 
                 Ray ray, HitRecord hit, ScatterRecord *scatter) {
    bool result = false;
    
    Material material = world->materials[mat.v];
    switch (material.type) {
        case MaterialType_Lambertian: {
            scatter->is_specular = false;
            scatter->attenuation = sample_texture(world, material.lambertian.albedo, &hit);
            scatter->pdf = cosine_pdf(hit.n);
            result = true;
        } break;
        case MaterialType_Metal: {
            Vec3 reflected = reflect(ray.dir, hit.n);
            Vec3 random = random_unit_sphere(entropy);
            Vec3 scattered = normalize(v3add(reflected, v3muls(random, material.metal.fuzz)));
            scatter->specular_dir = scattered;
            scatter->attenuation = sample_texture(world, material.metal.albedo, &hit);
            scatter->is_specular = true;
            result = true;
        } break;
        case MaterialType_Dielectric: {
            f32 refraction_ratio = material.dielectric.ir;
            if (hit.is_front_face) {
                refraction_ratio = 1.0f / refraction_ratio;
            }
                
            f32 cos_theta = min32(dot(v3neg(ray.dir), hit.n), 1.0f);
            f32 sin_theta = sqrt32(1.0f - cos_theta * cos_theta);
            
            Vec3 scattered;
            if ((refraction_ratio * sin_theta > 1.0f) || schlick(cos_theta, refraction_ratio) > random(entropy)) {
                scattered = reflect(ray.dir, hit.n);
            } else {
                scattered = refract(ray.dir, hit.n, refraction_ratio);
            }
            scatter->specular_dir = normalize(scattered);
            scatter->is_specular = true;
            scatter->attenuation = v3s(1);
        
            result = true;
        } break;
        case MaterialType_DiffuseLight: {
        } break;
        case MaterialType_Isotropic: {
            scatter->is_specular = true;
            scatter->specular_dir = random_unit_sphere(entropy);
            scatter->attenuation = sample_texture(world, material.isotropic.albedo, &hit);

            result = true;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

f32 
material_scattering_pdf(World *world, MaterialHandle mat, HitRecord hit, Ray ray) {
    f32 result = 0;
    
    Material material = world->materials[mat.v];
    switch(material.type) {
        case MaterialType_Lambertian: {
            f32 cosine = dot(hit.n, ray.dir);
            if (cosine > 0) {
                result = cosine / PI;
            }
        } break;
        case MaterialType_DiffuseLight: {} break;
        default: {
            assert(false);
        } break;
    }
    
    return result;
}

Vec3
material_emit(World *world, RandomSeries *entropy, MaterialHandle mat, HitRecord hit, Ray ray) {
    Vec3 result = {0};
    
    Material material = world->materials[mat.v];
    switch(material.type) {
        case MaterialType_DiffuseLight: {
            if (hit.is_front_face) {
                result = sample_texture(world, material.diffuse_light.emit, &hit);
            }
        } break;
        default: {
            
        } break;
    }

    return result;
}

bool
get_object_bounds(World *world, ObjectHandle obj_handle, Bounds3 *box) {
    bool result = false;
    
    Object *object = get_object(world, obj_handle);
    switch (object->type) {
        case ObjectType_ObjectList: {
            if (object->object_list.size) {
                Bounds3 temp_box;
                bool first_box = true;
                
                result = true;
                for (u64 object_index = 0;
                     object_index < object->object_list.size;
                     ++object_index) {
                    ObjectHandle test_object = object->object_list.a[object_index];
                    
                    if (!get_object_bounds(world, test_object, &temp_box)) {
                        result = false;
                        break;
                    }
                    
                    *box = first_box ? temp_box : bounds3_join(*box, temp_box);
                    first_box = false;
                }
            }
        } break;
        case ObjectType_Sphere: {
            Vec3 rv = v3s(object->sphere.r);
            box->min = v3sub(object->sphere.p, rv);
            box->max = v3add(object->sphere.p, rv);
            
            result = true;
        } break;
        case ObjectType_Triangle: {
            f32 min_x = min32(object->triangle.p[0].x, min32(object->triangle.p[1].x, object->triangle.p[2].x));
            f32 max_x = max32(object->triangle.p[0].x, max32(object->triangle.p[1].x, object->triangle.p[2].x));
            f32 min_y = min32(object->triangle.p[0].y, min32(object->triangle.p[1].y, object->triangle.p[2].y));
            f32 max_y = max32(object->triangle.p[0].y, max32(object->triangle.p[1].y, object->triangle.p[2].y));
            f32 min_z = min32(object->triangle.p[0].z, min32(object->triangle.p[1].z, object->triangle.p[2].z));
            f32 max_z = max32(object->triangle.p[0].z, max32(object->triangle.p[1].z, object->triangle.p[2].z));
            
            f32 epsilon = 0.001f;
            box->min = v3(min_x - epsilon, min_y - epsilon, min_z - epsilon);
            box->max = v3(max_x + epsilon, max_y + epsilon, max_z + epsilon);
            
            result = true;
        } break;
        case ObjectType_ConstantMedium: {
            result = get_object_bounds(world, object->constant_medium.boundary, box);
        } break;
        case ObjectType_Instance: {
            *box = object->instance.bounds;
            
            result = true;
        } break;
        case ObjectType_BVHNode: {
            *box = object->bvh_node.bounds;
            
            result = true;
        } break;
        case ObjectType_Box: {
            *box = object->box.bounds;
            
            result = true;
        } break;
        case ObjectType_FlipFace: {
            result = get_object_bounds(world, object->flip_face.obj, box);
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

f32 
get_object_pdf_value(World *world, RandomSeries *entropy, ObjectHandle object_handle, Vec3 orig, Vec3 v, RayCastStatistics *stats) {
    f32 result = 0;
    
    Object *object = get_object(world, object_handle);
    switch (object->type) {
        case ObjectType_Triangle: {
            HitRecord hit;
            if (object_hit(world, object_handle, make_ray(orig, v), &hit, 0.001f, INFINITY, entropy, stats)) {
                f32 surface_area = 0.5f * length(cross(v3sub(object->triangle.p[1], object->triangle.p[0]), 
                                                       v3sub(object->triangle.p[2], object->triangle.p[0])));
                f32 distance_squared = hit.t * hit.t * length_sq(v);
                f32 cosine = abs32(dot(v, hit.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_Sphere: {
            HitRecord hit;
            if (object_hit(world, object_handle, make_ray(orig, v), &hit, 0.001f, INFINITY, entropy, stats)) {
                f32 cos_theta_max = sqrt32(1 - object->sphere.r * object->sphere.r / length_sq(v3sub(object->sphere.p, orig)));
                f32 solid_angle = TWO_PI * (1 - cos_theta_max);
                
                result = 1.0f / solid_angle;
            }
        } break;
        case ObjectType_ObjectList: {
            f32 weight = 1.0f / object->object_list.size;
            f32 sum = 0;
            
            for (u32 object_index = 0;
                 object_index < object->object_list.size;
                 ++object_index) {
                sum += weight * get_object_pdf_value(world, entropy, object->object_list.a[object_index], orig, v, stats);        
            }
			result = sum;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
get_object_random(World *world, RandomSeries *entropy, ObjectHandle object_handle, Vec3 o) {
    Vec3 result = {0};
    Object *object = get_object(world, object_handle);
    switch (object->type) {
        case ObjectType_Triangle: {
            f32 u = random(entropy);
            f32 v = random(entropy);
            if (u + v >= 1.0f) {
                u = 1 - u;
                v = 1 - v;
            }
            
            result = v3add3(object->triangle.p[0], 
                            v3muls(v3sub(object->triangle.p[1], object->triangle.p[0]), u),
                            v3muls(v3sub(object->triangle.p[2], object->triangle.p[0]), v));
            result = v3sub(result, o);
        } break;
        case ObjectType_Sphere: {
            Vec3 dir = v3sub(object->sphere.p, o);
            f32 dist_sq = length_sq(dir);
            ONB uvw = onb_from_w(dir);
            result = onb_local(uvw, random_to_sphere(entropy, object->sphere.r, dist_sq));
        } break;
        case ObjectType_ObjectList: {
            u32 random_index = random_int(entropy, 0, object->object_list.size - 1);
            result = get_object_random(world, entropy, object->object_list.a[random_index], o);
        } break;
        default: {
            printf("%u\n", object->type);
            assert(false);
        }
    }
    
    return result;
}

bool 
object_hit(World *world, ObjectHandle obj_handle, Ray ray, 
           HitRecord *hit, f32 t_min, f32 t_max, 
           RandomSeries *entropy, RayCastStatistics *stats) {
    bool result = false;
    
    ++stats->object_collision_tests;
    
    Object *object = get_object(world, obj_handle);
    switch(object->type) {
        case ObjectType_Sphere: {
            Vec3 rel_orig = v3sub(ray.orig, object->sphere.p);
            f32 a = length_sq(ray.dir);
            f32 half_b = dot(rel_orig, ray.dir);
            f32 c = length_sq(rel_orig) - object->sphere.r * object->sphere.r;
            
            f32 discriminant = half_b * half_b - a * c;
            
            if (discriminant >= 0) {
                f32 root_term = sqrt32(discriminant);
                
                f32 tp = (-half_b + root_term) / a;
                f32 tn = (-half_b - root_term) / a;
                
                f32 t = tp;
                if ((tn > t_min) && (tn < tp)) {
                    t = tn;
                }
                
                if ((t > t_min) && (t < t_max)) {
                    hit->t = t;
                    hit->p = ray_at(ray, hit->t);
                    Vec3 outward_normal = v3divs(v3sub(hit->p, object->sphere.p), object->sphere.r);
                    hit_set_normal(hit, outward_normal, ray);
                    f32 u, v;
                    sphere_get_uv(outward_normal, &u, &v);
                    hit->u = u;
                    hit->v = v;
                    
                    hit->mat = object->sphere.mat;
                    result = true;
                }
            }
        } break;
        case ObjectType_Triangle: {
            ++stats->ray_triangle_collision_tests;
            
            Vec3 e1 = v3sub(object->triangle.p[1], object->triangle.p[0]);
            Vec3 e2 = v3sub(object->triangle.p[2], object->triangle.p[0]);
            Vec3 h = cross(ray.dir, e2);
            f32 a = dot(e1, h);
            
            if ((a < 0.001f) || (a > 0.001f)) {
                f32 f = 1.0f / a;
                Vec3 s = v3sub(ray.orig, object->triangle.p[0]);
                f32 u = f * dot(s, h);
                Vec3 q = cross(s, e1);
                f32 v = f * dot(ray.dir, q);
                f32 t = f * dot(e2, q);
                if ((0 < u) && (u < 1) && (v > 0) && (u + v < 1)) {
                    if ((t > t_min) && (t < t_max)) {
                        hit->t = t;
                        hit->p = ray_at(ray, hit->t);
                        // Vec3 outward_normal = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
                        Vec3 outward_normal = object->triangle.n;
                        hit_set_normal(hit, outward_normal, ray);
                        // @NOTE these are not actual uvs
                        hit->u = u;
                        hit->v = v;
                        
                        hit->mat = object->triangle.mat;
                        result = true;
                    }
                }
            }
        } break;
        case ObjectType_ObjectList: {
            bool has_hit_anything = false;
            
            f32 closest_so_far = t_max;
            for (u64 object_index = 0;
                object_index < object->object_list.size;
                ++object_index) {
                ObjectHandle test_object = object->object_list.a[object_index];
              
                HitRecord temp_hit = {0};  
                if (object_hit(world, test_object, ray, &temp_hit, t_min, closest_so_far, entropy, stats)) {
                    has_hit_anything = true;
                    closest_so_far = temp_hit.t;
                    *hit = temp_hit;
                }
            }
            
            result = has_hit_anything;
        } break;
        case ObjectType_ConstantMedium: {
            HitRecord hit1 = {0}, hit2 = {0};
            if (object_hit(world, object->constant_medium.boundary, ray, &hit1, -INFINITY, INFINITY, entropy, stats)) {
                if (object_hit(world, object->constant_medium.boundary, ray, &hit2, hit1.t + 0.0001f, INFINITY, entropy, stats)) {
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
                        f32 hit_dist = object->constant_medium.neg_inv_density * logf(random(entropy));
                        
                        if (hit_dist < distance_inside_boundary) {
                            hit->t = hit1.t + hit_dist;
                            hit->p = ray_at(ray, hit->t);
                            
                            // @NOTE can be not set bacuse not used
                            // hit->n = v3(1, 0, 0);
                            // hit->is_front_face = true;
                            hit->mat = object->constant_medium.phase_function;
                            
                            result = true;
                        } 
                    }
                }
            }
        } break;
        case ObjectType_Instance: {
            // @TODO something is wrong with hitting instances or bvhs
            Vec3 os_orig = mat4x4_mul_vec3(object->instance.w2o, ray.orig);
            Vec3 os_dir = mat4x4_as_3x3_mul_vec3(object->instance.w2o, ray.dir); 
            Ray os_ray = make_ray(os_orig, os_dir);
        
            result = object_hit(world, object->instance.object, os_ray, hit, t_min, t_max, entropy, stats);
            
            Vec3 ws_p = mat4x4_mul_vec3(object->instance.o2w, hit->p);
            Vec3 ws_n = normalize(mat4x4_as_3x3_mul_vec3(object->instance.o2w, hit->n));
            
            hit->p = ws_p;
            hit_set_normal(hit, ws_n, ray);   
        } break;
        case ObjectType_BVHNode: {
            if (bounds3_hit(object->bvh_node.bounds, ray, t_min, t_max)) {
                bool hit_left = object_hit(world, object->bvh_node.left, ray, hit, t_min, t_max, entropy, stats);
                bool hit_right = object_hit(world, object->bvh_node.right, ray, hit, t_min, hit_left ? hit->t : t_max, entropy, stats);
                result = hit_left || hit_right;
            }
        } break;
        case ObjectType_Box: {
            result = object_hit(world, object->box.sides_list, ray, hit, t_min, t_max, entropy, stats);
        } break;
        case ObjectType_FlipFace: {
            if (object_hit(world, object->flip_face.obj, ray, hit, t_min, t_max, entropy, stats)) {
                hit->is_front_face = !hit->is_front_face;
                result = true;
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

static f32 
pdf_value(World *world, RandomSeries *entropy, PDF pdf, Vec3 dir, RayCastStatistics *stats) {
    f32 result = 0;
    switch(pdf.type) {
        case PDFType_Cosine: {
            f32 cosine = dot(normalize(dir), pdf.cosine.uvw.w);
            if (cosine > 0) {
                result = cosine / PI;
            }
        } break;
        case PDFType_Object: {
            result = get_object_pdf_value(world, entropy, pdf.object.object, pdf.object.o, dir, stats);
        } break;
        case PDFType_Mixture: {
            result = lerp(pdf_value(world, entropy, *pdf.mix.p[0], dir, stats),
                          pdf_value(world, entropy, *pdf.mix.p[1], dir, stats),
                          0.5f);
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
} 

static Vec3 
pdf_generate(World *world, PDF pdf, RandomSeries *entropy) {
    Vec3 result = {0};
    switch(pdf.type) {
        case PDFType_Cosine: {
            result = onb_local(pdf.cosine.uvw, random_cosine_direction(entropy));
        } break;
        case PDFType_Object: {
            result = get_object_random(world, entropy, pdf.object.object, pdf.object.o);
        } break;
        case PDFType_Mixture: {
            if (random(entropy) < 0.5f) {
                result = pdf_generate(world, *pdf.mix.p[0], entropy);
            } else {
                result = pdf_generate(world, *pdf.mix.p[1], entropy);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
ray_cast(World *world, Ray ray, RandomSeries *entropy, i32 depth, RayCastStatistics *stats) {
    ++stats->bounce_count;
    
    HitRecord hit = {0};
    hit.t = INFINITY;
    
    if (depth <= 0) {
        return v3s(0);
    }
    
    if (!object_hit(world, world->object_list, ray, &hit, 0.001f, INFINITY, entropy, stats)) {
        return world->backgorund_color;
    }
    
    ScatterRecord srec = {0};
    Vec3 emitted = material_emit(world, entropy, hit.mat, hit, ray);
    if (!material_scatter(world, entropy, hit.mat, ray, hit, &srec)) {
        return emitted;
    }
    
    if (srec.is_specular) {
        return v3mul(srec.attenuation, ray_cast(world, make_ray(hit.p, srec.specular_dir), entropy, depth - 1, stats));
    }
    
    PDF lightp = object_pdf(world->lights, hit.p);
    PDF p = (PDF) {
        .type = PDFType_Mixture,
        .mix = {
            .p = { &lightp, &srec.pdf }
        }
    };
    
    Ray scattered = make_ray(hit.p, pdf_generate(world, p, entropy));
    f32 pdf_val = pdf_value(world, entropy, p, scattered.dir, stats);
    scattered.dir = normalize(scattered.dir);
    
    return v3add(emitted, v3mul(v3muls(srec.attenuation, material_scattering_pdf(world, hit.mat, hit, scattered)), 
                                v3divs(ray_cast(world, scattered, entropy, depth - 1, stats), pdf_val)));
}
