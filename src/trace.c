#include "trace.h"

static bool 
is_black(Vec3 color) {
    return !(isfinite(color.x) && isfinite(color.y) && isfinite(color.z));
}

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

void 
add_rect(World *world, ObjectHandle list, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, MaterialHandle mat) {
    add_object(world, list, object_triangle(world, p0, p1, p2, mat));    
    add_object(world, list, object_triangle(world, p0, p3, p2, mat));    
}

ObjectHandle 
add_poly_sphere(World *world, f32 r, u32 divs, MaterialHandle mat) {
    // generate points                                                                                                                                                                                      
    u32 numVertices = (divs - 1) * divs + 2; 
    // @TODO(hl): Memory pool!
    Vec3 *P = arena_alloc(&world->arena, numVertices * sizeof(Vec3)); 
    Vec3 *N = arena_alloc(&world->arena, numVertices * sizeof(Vec3)); 
    Vec2 *st = arena_alloc(&world->arena, numVertices *sizeof(Vec2)); 
	
    f32 u = -HALF_PI; 
    f32 v = -PI; 
    f32 du = PI / divs; 
    f32 dv = 2 * PI / divs; 
	
    P[0] = N[0] = v3(0, -r, 0); 
    u32 k = 1; 
    for (u32 i = 0;
         i < divs - 1;
         i++) { 
        u += du; 
        v = -PI; 
        for (u32 j = 0; 
             j < divs;
             j++) { 
            f32 x = r * cosf(u) * cosf(v); 
            f32 y = r * sinf(u); 
            f32 z = r * cosf(u) * sinf(v) ; 
            P[k] = N[k] = v3(x, y, z); 
            st[k].x = -v * 0.5 / PI + 0.5; 
            st[k].y = u / PI + 0.5; 
            v += dv, k++; 
        } 
    } 
    P[k] = N[k] = v3(0, r, 0); 
	
    u32 npolys = divs * divs; 
    u32 *faceIndex = arena_alloc(&world->arena, npolys * sizeof(u32)); 
    u32 *vertsIndex = arena_alloc(&world->arena, ((6 + (divs - 1) * 4) * divs * sizeof(u32))); 
	
    // create the connectivity lists                                                                                                                                                                        
    u32 vid = 1, numV = 0, l = 0; 
    k = 0; 
    for (u32 i = 0;
         i < divs;
         i++) 
    { 
        for (u32 j = 0;
             j < divs;
             j++) 
        { 
            if (i == 0) { 
                faceIndex[k++] = 3; 
                vertsIndex[l] = 0; 
                vertsIndex[l + 1] = j + vid; 
                vertsIndex[l + 2] = (j == (divs - 1)) ? vid : j + vid + 1; 
                l += 3; 
            } 
            else if (i == (divs - 1)) { 
                faceIndex[k++] = 3; 
                vertsIndex[l] = j + vid + 1 - divs; 
                vertsIndex[l + 1] = vid + 1; 
                vertsIndex[l + 2] = (j == (divs - 1)) ? vid + 1 - divs : j + vid + 2 - divs; 
                l += 3; 
            } 
            else { 
                faceIndex[k++] = 4; 
                vertsIndex[l] = j + vid + 1 - divs; 
                vertsIndex[l + 1] = j + vid + 1; 
                vertsIndex[l + 2] = (j == (divs - 1)) ? vid + 1 : j + vid + 2; 
                vertsIndex[l + 3] = (j == (divs - 1)) ? vid + 1 - divs : j + vid + 2 - divs; 
                l += 4; 
            } 
            numV++; 
        } 
        vid = numV; 
    } 
    
    PolygonMeshData pm;
    pm.nfaces = npolys;
    pm.vertices_per_face = faceIndex;
    pm.p = P;
    pm.n = N;
    pm.uv = st;
    return object_triangle_mesh_p(world, pm, mat);
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
        
        t_min = max(t0, t_min);
        t_max = min(t1, t_max);
        
        if (t_max < t_min) {
            return false;
        }
    }
    return true;
}

static bool 
triangle_hit(Vec3 p0, Vec3 p1, Vec3 p2, Ray ray, f32 *td, f32 *ud, f32 *vd, RayCastStatistics *stats) {
    ++stats->ray_triangle_collision_tests;
    bool result = false;
     
    Vec3 e1 = v3sub(p1, p0);
    Vec3 e2 = v3sub(p2, p0);
    Vec3 h = cross(ray.dir, e2);
    f32 a = dot(e1, h);
    
    if ((a < -0.001f) || (a > 0.001f)) {
        f32 f = 1.0f / a;
        Vec3 s = v3sub(ray.orig, p0);
        f32 u = f * dot(s, h);
        Vec3 q = cross(s, e1);
        f32 v = f * dot(ray.dir, q);
        f32 t = f * dot(e2, q);
        if ((0 < u) && (u < 1) && (v > 0) && (u + v < 1)) {
            *td = t;
            *ud = u;
            *vd = v;
            
            result = true;
        }
    }
    
    stats->ray_triangle_collision_test_succeses += result;
    return result;
}

static f32 
triangle_area(Vec3 p0, Vec3 p1, Vec3 p2) {
    return 0.5f * length(cross(v3sub(p1, p0), v3sub(p2, p0)));
}

#define BOX_COMPARATOR_SIGNATURE(_name) int _name(void *w, const void *a, const void *b)
typedef BOX_COMPARATOR_SIGNATURE(BoxComparator);

static int 
bounds3_compare(World *world, ObjectHandle a, ObjectHandle b, u32 axis) {
    Bounds3 b0 = get_object_bounds(world, a);
    Bounds3 b1 = get_object_bounds(world, b);
    return b0.min.e[axis] < b1.min.e[axis];
}

static BOX_COMPARATOR_SIGNATURE(bounds3_compare_x) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 0); }
static BOX_COMPARATOR_SIGNATURE(bounds3_compare_y) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 1); }
static BOX_COMPARATOR_SIGNATURE(bounds3_compare_z) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 2); }

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

static Bounds3 
transform_bounds(Bounds3 bi, Mat4x4 t) {
    Bounds3 bounds = bounds3i(mat4x4_mul_vec3(t, bi.min));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.min.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.max.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.min.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.max.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.max.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.min.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.max.y, bi.max.z)));
    return bounds;
}

Ray 
make_ray(Vec3 orig, Vec3 dir, f32 time) {
    return (Ray) {
        .orig = orig,
        .dir = dir,
        .time = time
    };
}

Vec3 
ray_at(Ray ray, f32 t) {
    Vec3 result = v3add(ray.orig, v3muls(ray.dir, t));
    return result;
}

void 
hit_set_normal(HitRecord *hrec, Vec3 n, Ray ray) {
    if (dot(ray.dir, n) <= 0.0f) {
        hrec->is_front_face = true;
        hrec->n = n;
    } else {
        hrec->is_front_face = false;
        hrec->n = v3neg(n);
    }
}

Camera 
camera_perspective(Vec3 look_from, Vec3 look_at, Vec3 v_up,
                   f32 aspect_ratio, f32 vfov,
                   f32 aperture, f32 focus_dist,
                   f32 time_min, f32 time_max) {
    Camera camera = {0};
    camera.type = CameraType_Perspective;
    
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
    
    camera.time_min = time_min;
    camera.time_max = time_max;    
    return camera;                         
}

Camera 
camera_orhographic(Vec3 look_from, Vec3 look_at, Vec3 v_up, 
                   f32 aspect_ratio, f32 focus_dist,
                   f32 time_min, f32 time_max) {
    Camera camera = {0};
    camera.type = CameraType_Orthographic;
    
    camera.time_min = time_min;
    camera.time_max = time_max; 
    
    camera.orig = look_from;
    camera.z = normalize(v3sub(look_from, look_at));
    camera.x = normalize(cross(v_up, camera.z));
    camera.y = normalize(cross(camera.z, camera.x));
     
    f32 viewport_height = 1.0f;
    f32 viewport_width  = 1.0f;
    if (aspect_ratio > 1) {
        viewport_height = 1.0f / aspect_ratio;
    } else if (aspect_ratio < 1) {
        viewport_width = aspect_ratio;
    }
    
    camera.horizontal = v3muls(camera.x, focus_dist * viewport_width);
    camera.vertical   = v3muls(camera.y, focus_dist * viewport_height);
    
    camera.lower_left_corner = camera.orig;
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.horizontal, 0.5f));
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.vertical, 0.5f));
    camera.lower_left_corner = v3sub(camera.lower_left_corner, v3muls(camera.z, focus_dist));   
    
    
    return camera;                          
}

Camera 
camera_environment(Vec3 look_from, Vec3 look_at, Vec3 v_up, f32 time_min, f32 time_max) {
    Camera camera = {0};
    camera.type = CameraType_Environment;

    camera.orig = look_from;
    camera.z = normalize(v3sub(look_from, look_at));
    camera.x = normalize(cross(v_up, camera.z));
    camera.y = normalize(cross(camera.z, camera.x));

    camera.time_min = time_min;
    camera.time_max = time_max;   
      
    return camera;
}

Ray 
camera_make_ray(Camera *camera, RandomSeries *entropy, f32 u, f32 v) {
    Vec3 dir, orig;
    switch(camera->type) {
        case CameraType_Perspective: {   
            Vec3 rd = v3muls(random_unit_disk(entropy), camera->lens_radius);
            Vec3 offset = v3add(v3muls(camera->x, rd.x), v3muls(camera->y, rd.y));
    
            dir = camera->lower_left_corner;
            dir = v3add(dir, v3muls(camera->horizontal, u));
            dir = v3add(dir, v3muls(camera->vertical, v));
            dir = v3sub(dir, camera->orig);
            dir = v3sub(dir, offset);
            dir = normalize(dir);
            
            orig = v3add(camera->orig, offset);
        } break;
        case CameraType_Orthographic: {
            orig = camera->lower_left_corner;
            orig = v3add(orig, v3muls(camera->horizontal, u));
            orig = v3add(orig, v3muls(camera->vertical, v));
            orig = v3add(orig, camera->orig);
            dir = v3neg(camera->z);
        } break;
        case CameraType_Environment: {
            f32 theta = v * PI;
            f32 phi = u * TWO_PI;
            
            Vec3 angle = v3(sinf(theta) * cosf(phi), -cosf(theta), sinf(theta) * sinf(phi));
            // angle = v3add3(v3muls(camera->x, angle.x), v3muls(camera->y, angle.y), v3muls(camera->z, angle.z));
            angle = normalize(angle);
            dir = angle;
            orig = camera->orig;
        } break;
        INVALID_DEFAULT_CASE;
    }
    f32 time = random_uniform(entropy, camera->time_min, camera->time_max);
    Ray ray = make_ray(orig, dir, time);
    
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
	lst.arena = arena;

#if RAY_INTERNAL
    lst.is_initialized = true;
#endif 
    return lst;
}

void
add_object_to_list(ObjectList *list, ObjectHandle o) {
#if RAY_INTERNAL
    assert(list->is_initialized);
#endif 
    EXPAND_IF_NEEDED(list->arena, list->a, list->size, list->capacity);
    
    list->a[list->size++] = o;   
}

void
object_list_shrink_to_fit(ObjectList *list) {
#if RAY_INTERNAL
    assert(list->is_initialized);
#endif 
    SHRINK_TO_FIT(list->arena, list->a, list->size, list->capacity);
    
}

ObjectHandle 
object_list_get(ObjectList *list, u64 index) {
#if RAY_INTERNAL
    assert(list->is_initialized);
    assert(index < list->size);
#endif
    
    return list->a[index];
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
            .obj = obj
        }
    };
}

PDF 
mixture_pdf(PDF *p1, PDF *p2) {
    PDF pdf;
    pdf.type = PDFType_Mixture;
    pdf.mix.p[0] = p1;
    pdf.mix.p[1] = p2;
    return pdf;
}

void 
world_init(World *world) {
    memset(world, 0, sizeof(*world));
    
    world->arena.data_capacity = MEGABYTES(16);
    world->arena.data = malloc(world->arena.data_capacity);
    
    world->obj_list = object_list(world);
    world->important_objects = object_list(world);
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
new_object(World *world, Object obj) {
    EXPAND_IF_NEEDED(&world->arena, world->objects, world->objects_size, world->objects_capacity);
    
    ObjectHandle handle = { world->objects_size };
    world->objects[world->objects_size++] = obj;
    
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

ObjectHandle
add_object(World *world, ObjectHandle list_handle, ObjectHandle obj) {
    Object *list = get_object(world, list_handle);
    assert(list->type = ObjectType_ObjectList);
 
    add_object_to_list(&list->obj_list, obj);   
    return obj;
}

ObjectHandle 
add_object_to_world(World *world, ObjectHandle o) {
    add_object(world, world->obj_list, o);
    return o;
}

ObjectHandle 
add_important_object(World *world, ObjectHandle obj) {
    world->has_importance_sampling = true;
    return add_object(world, world->important_objects, obj);
}

TextureHandle 
texture_solid(World *world, Vec3 c) {
    Texture texture;
    texture.type = TextureType_Solid;
    texture.solid.c = c;
    
    return new_texture(world, texture);    
}

TextureHandle 
texture_checkerboard(World *world, TextureHandle t1, TextureHandle t2) {
    Texture texture;
    texture.type = TextureType_Checkerboard;
    texture.checkerboard.t1 = t1;    
    texture.checkerboard.t2 = t2;    
    
    return new_texture(world, texture);
}

TextureHandle 
texture_checkerboard3d(World *world, TextureHandle t1, TextureHandle t2) {
    Texture texture;
    texture.type = TextureType_Checkerboard3D;
    texture.checkerboard3d.t1 = t1;    
    texture.checkerboard3d.t2 = t2;    
    
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
material_metal(World *world, TextureHandle albedo, f32 roughness) {
    assert(roughness <= 1.0f);
    
    Material material;
    material.type = MaterialType_Metal;
    material.metal.albedo = albedo;
    material.metal.roughness = roughness;
    
    return new_material(world, material);    
}

MaterialHandle 
material_plastic(World *world, TextureHandle albedo, f32 roughness) {
    assert(roughness <= 1.0f);
    
    Material material;
    material.type = MaterialType_Plastic;
    material.plastic.albedo = albedo;
    material.plastic.roughness = roughness;
    
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
material_diffuse_light(World *world, TextureHandle t, LightFlags flags) {
    Material material;
    material.type = MaterialType_DiffuseLight;
    material.diffuse_light.emit = t;
    material.diffuse_light.flags = flags;
    
    return new_material(world, material);    
}

ObjectHandle 
object_list(World *world) {
    Object obj;
    obj.type = ObjectType_ObjectList;
    obj.obj_list = object_list_init(&world->arena, 0);
    
    return new_object(world, obj);        
}

ObjectHandle 
object_sphere(World *world, Vec3 p, f32 r, MaterialHandle mat) {
    Object obj;
    obj.type = ObjectType_Sphere;
    obj.sphere.mat = mat;
    obj.sphere.p = p;
    obj.sphere.r = r;
    
    return new_object(world, obj);        
}

ObjectHandle 
object_disk(World *world, Vec3 p, Vec3 n, f32 r, MaterialHandle mat) {
    Object obj;
    obj.type = ObjectType_Disk;
    obj.disk.mat = mat;
    obj.disk.p = p;
    obj.disk.n = n;
    obj.disk.r = r;
    
    return new_object(world, obj);   
}

ObjectHandle 
object_transform(World *world, ObjectHandle objh, Transform t) {
    Bounds3 ob = get_object_bounds(world, objh);
    
    Bounds3 bounds = bounds3i(mat4x4_mul_vec3(t.o2w, ob.min));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.max.x, ob.min.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.min.x, ob.max.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.min.x, ob.min.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.min.x, ob.max.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.max.x, ob.max.y, ob.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.max.x, ob.min.y, ob.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t.o2w, v3(ob.max.x, ob.max.y, ob.max.z)));
    
    Object obj;
    obj.type = ObjectType_Transform;
    obj.transform.bounds = bounds;
    obj.transform.t = t;
    obj.transform.obj = objh;
    
    return new_object(world, obj);        
}

ObjectHandle 
object_triangle(World *world, Vec3 p0, Vec3 p1, Vec3 p2, MaterialHandle mat) {
    Vec3 n = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
    
    Object obj;
    obj.type = ObjectType_Triangle;
    obj.triangle.p[0] = p0;
    obj.triangle.p[1] = p1;
    obj.triangle.p[2] = p2;
    obj.triangle.n = n;
    obj.triangle.mat = mat;
    
    return new_object(world, obj);        
}

ObjectHandle 
object_box(World *world, Vec3 min, Vec3 max, MaterialHandle mat) {
    Object obj;
    obj.type = ObjectType_Box;
    obj.box.bounds = bounds3(min, max);
    obj.box.sides_list = object_list(world);
    add_box(world, obj.box.sides_list, min, max, mat);
    object_list_shrink_to_fit(&get_object(world, obj.box.sides_list)->obj_list);
    
    return new_object(world, obj);        
}

ObjectHandle 
object_constant_medium(World *world, f32 d, MaterialHandle phase, ObjectHandle bound) {
    Object obj;
    obj.type = ObjectType_ConstantMedium;
    obj.constant_medium.boundary = bound;
    obj.constant_medium.neg_inv_density = -1.0f / d;
    obj.constant_medium.phase_function = phase;
    
    return new_object(world, obj);        
}

ObjectHandle 
object_bvh_node(World *world, ObjectHandle *objs, i64 n) {
    Object obj;
    obj.type = ObjectType_BVHNode;
    
    TempMemory temp_mem = temp_memory_begin(&world->arena);
    Bounds3 *objs_bounds = arena_alloc(&world->arena, sizeof(Bounds3) * n); 
    f32 *left_area  = arena_alloc(&world->arena, sizeof(f32) * n); 
    f32 *right_area = arena_alloc(&world->arena, sizeof(f32) * n); 
    
    Bounds3 main_bounds = bounds3empty();
    for (u32 obj_index = 0;
         obj_index < n;
         ++obj_index) {
        ObjectHandle obj_handle = objs[obj_index];
        Bounds3 temp_bounds = get_object_bounds(world, obj_handle);
        main_bounds = bounds3_join(main_bounds, temp_bounds);
    }
    
    u32 axis = bounds3s_longest_axis(main_bounds);
    if (axis == 0) {
        qsort_s(objs, n, sizeof(ObjectHandle), bounds3_compare_x, world);
    } else if (axis == 1) {
        qsort_s(objs, n, sizeof(ObjectHandle), bounds3_compare_y, world);
    } else {
        qsort_s(objs, n, sizeof(ObjectHandle), bounds3_compare_z, world);
    }
    
    for (u32 obj_index = 0;
         obj_index < n;
         ++obj_index) {
        ObjectHandle obj_handle = objs[obj_index];
        objs_bounds[obj_index] = get_object_bounds(world, obj_handle);
    }
    
    Bounds3 left_bounds = bounds3empty();
    for (u32 obj_index = 0;
         obj_index < n;
         ++obj_index) {
        left_bounds = bounds3_join(left_bounds, objs_bounds[obj_index]);
        left_area[obj_index] = bound3s_surface_area(left_bounds);
    }
    Bounds3 right_bounds = bounds3empty();
    for (i32 obj_index = n - 1;
         obj_index > 0;
         --obj_index) {
        right_bounds = bounds3_join(right_bounds, objs_bounds[obj_index]);
        right_area[obj_index] = bound3s_surface_area(right_bounds);        
    }
    
    f32 min_sah = INFINITY;
    u32 min_sah_index;
    for (u32 object_index = 0;
         object_index < n - 1;
         ++object_index) {
        f32 sah = object_index * left_area[object_index] + (n - object_index - 1) * right_area[object_index + 1];
        if (sah < min_sah) {
            min_sah_index = object_index;
            min_sah = sah;
        }        
    }
    
    if (min_sah_index == 0) {
        obj.bvh_node.left = objs[0];
    } else {
        obj.bvh_node.left = object_bvh_node(world, objs, min_sah_index + 1);
    }
    
    if (min_sah_index == n - 2) {
        obj.bvh_node.right = objs[min_sah_index + 1];
    } else {
        obj.bvh_node.right = object_bvh_node(world, objs + min_sah_index + 1, n - min_sah_index - 1);
    }
    
    temp_memory_end(temp_mem);
    obj.bvh_node.bounds = main_bounds;
    
    return new_object(world, obj);        
}

ObjectHandle 
object_animated_transform(World *world, ObjectHandle objh, f32 time0, f32 time1,
                          Vec3 t0, Vec3 t1, Quat4 r0, Quat4 r1) {
    Object obj;
    obj.type = ObjectType_AnimatedTransform;
    obj.animated_transform.time[0] = time0;
    obj.animated_transform.time[1] = time1;
    obj.animated_transform.t[0] = t0;
    obj.animated_transform.t[1] = t1;
    obj.animated_transform.r[0] = r0;
    obj.animated_transform.r[1] = r1;
    obj.animated_transform.obj = objh;
    
    Bounds3 obj_bounds = get_object_bounds(world, objh);
    
    f32 min_d = min32(min32(obj_bounds.min.x, obj_bounds.min.y), obj_bounds.min.z);
    f32 max_d = max32(min32(obj_bounds.max.x, obj_bounds.max.y), obj_bounds.max.z);
    Bounds3 bounds_box = bounds3(v3s(min_d), v3s(max_d));
    
    Mat4x4 m0 = MAT4X4_IDENTITY, m1 = MAT4X4_IDENTITY;
    m0 = mat4x4_mul(m0, mat4x4_translate(t0));
    m1 = mat4x4_mul(m0, mat4x4_translate(t1));
    m0 = mat4x4_mul(m0, mat4x4_from_quat4(r0));
    m1 = mat4x4_mul(m1, mat4x4_from_quat4(r1));
    
    Bounds3 b0 = transform_bounds(bounds_box, m0);
    Bounds3 b1 = transform_bounds(bounds_box, m1);
    Bounds3 bounds = bounds3_join(b0, b1);
    obj.animated_transform.bounds = bounds;
    
    return new_object(world, obj);
}

ObjectHandle 
object_triangle_mesh_pt(World *world, PolygonMeshData pm, MaterialHandle mat, Transform transform) {
    Object obj;
    obj.type = ObjectType_TriangleMesh;
    
    u64 vertex_index_cursor = 0;
    u32 max_vert_index = 0;
    u64 triangle_count = 0;
    for (u32 face_index = 0;
         face_index < pm.nfaces;
         ++face_index) {
        u32 vertices_in_face = pm.vertices_per_face[face_index];
        triangle_count += vertices_in_face - 2;
        for (u32 vertex_in_face_index = 0;
             vertex_in_face_index < pm.vertices_per_face[face_index];
             ++vertex_in_face_index) {
            u32 vert_index = pm.vertex_indices[vertex_index_cursor + vertex_in_face_index];    
            if (vert_index > max_vert_index) {
                max_vert_index = vert_index;
            }    
        }
        vertex_index_cursor += vertices_in_face;
    }
    u64 vertex_count = max_vert_index + 1;
    
    obj.triangle_mesh.ntrig = triangle_count;
    obj.triangle_mesh.nvert = vertex_count;
    
    Bounds3 bounds = bounds3empty();
    obj.triangle_mesh.p = arena_alloc(&world->arena, vertex_count * sizeof(Vec3));
    obj.triangle_mesh.n = arena_alloc(&world->arena, vertex_count * sizeof(Vec3));
    obj.triangle_mesh.uv = arena_alloc(&world->arena, vertex_count * sizeof(Vec2));
    for (u32 vertex_index = 0;
         vertex_index < vertex_count;
         ++vertex_index) {
        obj.triangle_mesh.p[vertex_index] = mat4x4_mul_vec3(transform.o2w, pm.p[vertex_index]);
        obj.triangle_mesh.n[vertex_index] = mat4x4_as_3x3_mul_vec3(transform.o2w, pm.n[vertex_index]);
        obj.triangle_mesh.uv[vertex_index] = pm.uv[vertex_index];
        bounds = bounds3_extend(bounds, obj.triangle_mesh.p[vertex_index]);
    }
    obj.triangle_mesh.bounds = bounds;
    obj.triangle_mesh.tri_indices = arena_alloc(&world->arena, triangle_count * 3 * sizeof(u32));
    
    f32 surface_area = 0;
    vertex_index_cursor = 0;
    u64 index_cursor = 0;
    for (u32 face_index = 0;
         face_index < pm.nfaces;
         ++face_index) {
        for (u32 triangle_in_face_index = 0;
             triangle_in_face_index < pm.vertices_per_face[face_index] - 2;
             ++triangle_in_face_index) {
            obj.triangle_mesh.tri_indices[index_cursor    ] = pm.vertex_indices[vertex_index_cursor];
            obj.triangle_mesh.tri_indices[index_cursor + 1] = pm.vertex_indices[vertex_index_cursor + triangle_in_face_index + 1];
            obj.triangle_mesh.tri_indices[index_cursor + 2] = pm.vertex_indices[vertex_index_cursor + triangle_in_face_index + 2];
            
            surface_area += triangle_area(obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[index_cursor]],
                                          obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[index_cursor + 1]],
                                          obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[index_cursor + 2]]);
                                    
            index_cursor += 3;
        }        
        vertex_index_cursor += pm.vertices_per_face[face_index];
    }
    obj.triangle_mesh.mat = mat;
    obj.triangle_mesh.surface_area = surface_area;
    
    return new_object(world, obj);
}

ObjectHandle 
object_triangle_mesh_tt(World *world, TriangleMeshData tm, MaterialHandle mat, Transform transform) {
    Object obj;
    obj.type = ObjectType_TriangleMesh;
    
    obj.triangle_mesh.ntrig = tm.ntrig;
    obj.triangle_mesh.nvert = tm.nvert;
    
    u64 vertex_count = tm.nvert;
    Bounds3 bounds = bounds3empty();
    obj.triangle_mesh.p = arena_alloc(&world->arena, vertex_count * sizeof(Vec3));
    obj.triangle_mesh.n = arena_alloc(&world->arena, vertex_count * sizeof(Vec3));
    obj.triangle_mesh.uv = arena_alloc(&world->arena, vertex_count * sizeof(Vec2));
    for (u32 vertex_index = 0;
         vertex_index < vertex_count;
         ++vertex_index) {
        obj.triangle_mesh.p[vertex_index] = mat4x4_mul_vec3(transform.o2w, tm.p[vertex_index]);
        obj.triangle_mesh.n[vertex_index] = mat4x4_as_3x3_mul_vec3(transform.o2w, tm.n[vertex_index]);
        obj.triangle_mesh.uv[vertex_index] = tm.uv[vertex_index];
        bounds = bounds3_extend(bounds, obj.triangle_mesh.p[vertex_index]);
    }
    obj.triangle_mesh.bounds = bounds;
    obj.triangle_mesh.tri_indices = arena_copy(&world->arena, tm.tri_indices, tm.ntrig * 3 * sizeof(u32));
    
    f32 surface_area = 0;
    for (u32 triangle_index = 0;
         triangle_index < tm.ntrig;
         ++triangle_index) {
        surface_area += triangle_area(obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[triangle_index * 3]],
                                      obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[triangle_index * 3 + 1]],
                                      obj.triangle_mesh.p[obj.triangle_mesh.tri_indices[triangle_index * 3 + 2]]);
    }
    obj.triangle_mesh.mat = mat;
    obj.triangle_mesh.surface_area = surface_area;
    
    return new_object(world, obj);
}

Vec3
sample_texture(World *world, TextureHandle handle, HitRecord *hrec) {
    Vec3 result;
    
    Texture *texture = world->textures + handle.v; 
    switch(texture->type) {
        case TextureType_Solid: {
            result = texture->solid.c;
        } break;
        case TextureType_Checkerboard: {
            if (((i32)floorf(hrec->u) + (i32)floorf(hrec->v)) % 2 == 0) {
                result = sample_texture(world, texture->checkerboard.t1, hrec);
            } else {
                result = sample_texture(world, texture->checkerboard.t2, hrec);
            }
        } break;
        case TextureType_Checkerboard3D: {
            if (((i32)floorf(hrec->p.x) + (i32)floorf(hrec->p.y) + (i32)floorf(hrec->p.z)) % 2 == 0) {
                result = sample_texture(world, texture->checkerboard.t1, hrec);
            } else {
                result = sample_texture(world, texture->checkerboard.t2, hrec);
            }
        } break;
        case TextureType_Image: {
            f32 u = clamp(hrec->u, 0, 1);
            f32 v = clamp(hrec->v, 0, 1);
            
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
            // f32 noise = perlin_turb(&texture->perlin.p, v3muls(hrec->p, texture->perlin.s), 7);
            f32 noise = 0.5f * (1 + sinf(texture->perlin.s * hrec->p.z + 10 * perlin_turb(&texture->perlin.p, hrec->p, 7)));
            result = v3muls(v3s(1), noise);
        } break;
        case TextureType_UV: {
            result = v3(hrec->u, hrec->v, 0);
        } break;
        case TextureType_Normal: {
            result = v3muls(v3add(hrec->n, v3s(1)), 0.5f);
        } break;
        INVALID_DEFAULT_CASE;
    }
    return result;
} 

bool 
material_scatter(World *world, Ray ray, HitRecord hrec, 
                 ScatterRecord *scatter, RayCastData data) {
    bool result = false;
    
    Material *material = get_material(world, hrec.mat);
    switch (material->type) {
        case MaterialType_Lambertian: {
            scatter->is_specular = false;
            scatter->attenuation = sample_texture(world, material->lambertian.albedo, &hrec);
            scatter->pdf = cosine_pdf(hrec.n);
            result = true;
        } break;
        case MaterialType_Metal: {
            Vec3 reflected = reflect(ray.dir, hrec.n);
            Vec3 random = random_unit_sphere(data.entropy);
            Vec3 scattered = normalize(v3add(reflected, v3muls(random, material->metal.roughness)));
            scatter->specular_dir = scattered;
            scatter->attenuation = sample_texture(world, material->metal.albedo, &hrec);
            scatter->is_specular = true;
            result = true;
        } break;
        case MaterialType_Plastic: {
            // @NOTE this is not physically correct, we just choose randomly diffuse or specular scattering
            if (random(data.entropy) > 0.6) {
                Vec3 reflected = reflect(ray.dir, hrec.n);
                Vec3 random = random_unit_sphere(data.entropy);
                Vec3 scattered = normalize(v3add(reflected, v3muls(random, material->plastic.roughness)));
                scatter->specular_dir = scattered;
                scatter->attenuation = sample_texture(world, material->plastic.albedo, &hrec);
                scatter->is_specular = true;
            } else {
                scatter->attenuation = sample_texture(world, material->plastic.albedo, &hrec);
                scatter->pdf = cosine_pdf(hrec.n);
            }
            result = true;
        } break;
        case MaterialType_Dielectric: {
            f32 refraction_ratio = material->dielectric.ir;
            if (hrec.is_front_face) {
                refraction_ratio = 1.0f / refraction_ratio;
            }
                
            f32 cos_theta = min32(-dot(ray.dir, hrec.n), 1.0f);
            f32 sin_theta = sqrt32(1.0f - cos_theta * cos_theta);
            
            Vec3 scattered;
            if ((refraction_ratio * sin_theta > 1.0f) || schlick(cos_theta, refraction_ratio) > random(data.entropy)) {
                scattered = reflect(ray.dir, hrec.n);
            } else {
                scattered = refract(ray.dir, hrec.n, refraction_ratio);
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
            scatter->specular_dir = random_unit_sphere(data.entropy);
            scatter->attenuation = sample_texture(world, material->isotropic.albedo, &hrec);

            result = true;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

f32 
material_scattering_pdf(World *world, Ray ray, HitRecord hrec) {
    f32 result = 0;
    
    Material *material = get_material(world, hrec.mat);
    switch(material->type) {
        case MaterialType_Lambertian: 
        case MaterialType_Plastic: {
            f32 cosine = dot(hrec.n, normalize(ray.dir));
            if (cosine > 0) {
                result = cosine / PI;
            }
        } break;
        case MaterialType_DiffuseLight: {} break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
material_emit(World *world, Ray ray, HitRecord hrec, RayCastData data) {
    Vec3 result = {0};
    
    Material *material = get_material(world, hrec.mat);
    switch(material->type) {
        case MaterialType_DiffuseLight: {
            bool is_front_face = hrec.is_front_face;
            if (material->diffuse_light.flags & LightFlags_FlipFace) {
                is_front_face = !is_front_face;
            }
            
            if (!(material->diffuse_light.flags & LightFlags_BothSided) && !is_front_face) {
                break;
            }
            
            result = sample_texture(world, material->diffuse_light.emit, &hrec);
        } break;
        default: {
            
        } break;
    }

    return result;
}

Bounds3 
get_object_bounds(World *world, ObjectHandle obj_handle) {
    Bounds3 result = bounds3empty();
    
    Object *obj = get_object(world, obj_handle);
    switch (obj->type) {
        case ObjectType_ObjectList: {
            if (obj->obj_list.size) {
                bool is_first_box = true;
                for (u64 obj_index = 0;
                     obj_index < obj->obj_list.size;
                     ++obj_index) {
                    ObjectHandle test_object = object_list_get(&obj->obj_list, obj_index);
                    Bounds3 temp_box = get_object_bounds(world, test_object);
                    result = is_first_box ? temp_box : bounds3_join(result, temp_box);
                    is_first_box = false;
                }
            }
        } break;
        case ObjectType_Sphere: {
            Vec3 rv = v3s(obj->sphere.r);
            result.min = v3sub(obj->sphere.p, rv);
            result.max = v3add(obj->sphere.p, rv);
        } break;
        case ObjectType_Triangle: {
            Vec3 epsilon = v3s(0.001f);
            result = bounds3empty();
            result = bounds3_extend(result, obj->triangle.p[0]);
            result = bounds3_extend(result, obj->triangle.p[1]);
            result = bounds3_extend(result, obj->triangle.p[2]);
            result.min = v3sub(result.min, epsilon);
            result.max = v3add(result.max, epsilon);
        } break;
        case ObjectType_ConstantMedium: {
            result = get_object_bounds(world, obj->constant_medium.boundary);
        } break;
        case ObjectType_Transform: {
            result = obj->transform.bounds;
        } break;
        case ObjectType_AnimatedTransform: {
            result = obj->animated_transform.bounds;
        } break;
        case ObjectType_BVHNode: {
            result = obj->bvh_node.bounds;
        } break;
        case ObjectType_Box: {
            result = obj->box.bounds;
        } break;
        case ObjectType_TriangleMesh: {
            result = obj->triangle_mesh.bounds;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

f32 
get_object_pdf_value(World *world, ObjectHandle object_handle, Vec3 orig, Vec3 v,
                     RayCastData data){
    f32 result = 0;
    
    Object *obj = get_object(world, object_handle);
    switch (obj->type) {
        case ObjectType_Triangle: {
            HitRecord hrec;
            if (object_hit(world, make_ray(orig, v, 0), object_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 surface_area = 0.5f * length(cross(v3sub(obj->triangle.p[1], obj->triangle.p[0]), 
                                                       v3sub(obj->triangle.p[2], obj->triangle.p[0])));
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_Sphere: {
            HitRecord hrec;
            if (object_hit(world, make_ray(orig, normalize(v), 0), object_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 cos_theta_max = sqrt32(1 - obj->sphere.r * obj->sphere.r / length_sq(v3sub(obj->sphere.p, orig)));
                f32 solid_angle = TWO_PI * (1 - cos_theta_max);
                
                result = 1.0f / solid_angle;
            }
        } break;
        case ObjectType_Disk: {
            HitRecord hrec;
            if (object_hit(world, make_ray(orig, v, 0), object_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 surface_area = PI * obj->disk.r * obj->disk.r;
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_TriangleMesh: {
            HitRecord hrec;
            if (object_hit(world, make_ray(orig, normalize(v), 0), object_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 surface_area = obj->triangle_mesh.surface_area;
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_ObjectList: {
            f32 weight = 1.0f / obj->obj_list.size;
            f32 sum = 0;
            
            for (u32 obj_index = 0;
                 obj_index < obj->obj_list.size;
                 ++obj_index) {
                sum += weight * get_object_pdf_value(world, obj->obj_list.a[obj_index], orig, v, data);        
            }
			result = sum;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
get_object_random(World *world, ObjectHandle object_handle, Vec3 o, RayCastData data) {
    Vec3 result = {0};
    Object *obj = get_object(world, object_handle);
    switch (obj->type) {
        case ObjectType_Triangle: {
            f32 u = random(data.entropy);
            f32 v = random(data.entropy);
            if (u + v >= 1.0f) {
                u = 1 - u;
                v = 1 - v;
            }
            
            result = v3add3(obj->triangle.p[0], 
                            v3muls(v3sub(obj->triangle.p[1], obj->triangle.p[0]), u),
                            v3muls(v3sub(obj->triangle.p[2], obj->triangle.p[0]), v));
            result = v3sub(result, o);
        } break;
        case ObjectType_Disk: {
            ONB uvw = onb_from_w(obj->disk.n);
            result = v3add(onb_local(uvw, v3muls(random_unit_disk(data.entropy), obj->disk.r)), obj->disk.p);
        } break;
        case ObjectType_TriangleMesh: {
            for (;;) {
                Bounds3 b = obj->triangle_mesh.bounds;
                Vec3 p = v3(random_uniform(data.entropy, b.min.x, b.max.x), 
                            random_uniform(data.entropy, b.min.y, b.max.y),
                            random_uniform(data.entropy, b.min.z, b.max.z));
                u32 intersection_count = 0;
                // @NOTE this needs a lot of optimizing
                // Ray ray = make_ray(o, normalize(v3sub(p, o)), 0);
                // f32 t_min = -INFINITY;
                // for (;;) {
                //     HitRecord hrec;
                //     if (object_hit(world, ray, object_handle, t_min, INFINITY, &hrec, data)) {
                //         ++intersection_count;
                //         t_min = hrec.t;
                //     }
                // }
                
                if (intersection_count % 2 == 0) {
                    result = p;
                    break;
                }
            }
        } break;
        case ObjectType_Sphere: {
            Vec3 dir = v3sub(obj->sphere.p, o);
            f32 dist_sq = length_sq(dir);
            ONB uvw = onb_from_w(dir);
            result = onb_local(uvw, random_to_sphere(data.entropy, obj->sphere.r, dist_sq));
        } break;
        case ObjectType_ObjectList: {
            if (obj->obj_list.size) {
                u32 random_index = random_int(data.entropy, obj->obj_list.size);
                result = get_object_random(world, object_list_get(&obj->obj_list, random_index), o, data);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

bool 
object_hit(World *world, Ray ray, ObjectHandle obj_handle, f32 t_min, f32 t_max,
           HitRecord *hrec, RayCastData data) {
    bool result = false;
    
    ++data.stats->object_collision_tests;
    
    Object *obj = get_object(world, obj_handle);
    switch(obj->type) {
        case ObjectType_Sphere: {
            Vec3 rel_orig = v3sub(ray.orig, obj->sphere.p);
            f32 a = length_sq(ray.dir);
            f32 half_b = dot(rel_orig, ray.dir);
            f32 c = length_sq(rel_orig) - obj->sphere.r * obj->sphere.r;
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
                    hrec->t = t;
                    hrec->p = ray_at(ray, hrec->t);
                    Vec3 outward_normal = v3divs(v3sub(hrec->p, obj->sphere.p), obj->sphere.r);
                    hit_set_normal(hrec, outward_normal, ray);
                    f32 u, v;
                    sphere_get_uv(outward_normal, &u, &v);
                    hrec->u = u;
                    hrec->v = v;
                    
                    hrec->mat = obj->sphere.mat;
                    result = true;
                }
            }
        } break;
        case ObjectType_Disk: {
            f32 d = dot(obj->disk.n, ray.dir);
            if ((d < -0.001f) || (d > 0.001f)) {
                Vec3 ro = v3sub(obj->disk.p, ray.orig); 
                f32 t = dot(ro, obj->disk.n) / d;
                Vec3 hp = ray_at(ray, t);
                f32 dtcsq = length_sq(v3sub(hp, obj->disk.p));
                if ((t > t_min) && (t < t_max) && (dtcsq < obj->disk.r * obj->disk.r)) {
                    hrec->t = t;
                    hrec->p = hp;
                    hit_set_normal(hrec, obj->disk.n, ray);
                    
                    hrec->mat = obj->disk.mat;
                    result = true;
                }
            }
        } break;
        case ObjectType_Triangle: {
            ++data.stats->ray_triangle_collision_tests;
           
           f32 t, u, v;
           if (triangle_hit(obj->triangle.p[0], obj->triangle.p[1], obj->triangle.p[2], ray, 
                &t, &u, &v, data.stats)) {
                if ((t > t_min) && (t < t_max)) {
                    hrec->t = t;
                    hrec->p = ray_at(ray, hrec->t);
                    // Vec3 outward_normal = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
                    Vec3 outward_normal = obj->triangle.n;
                    hit_set_normal(hrec, outward_normal, ray);
                    // @NOTE these are not actual uvs
                    hrec->u = u;
                    hrec->v = v;
                    
                    hrec->mat = obj->triangle.mat;
                    result = true;
                }
           }
        } break;
        case ObjectType_ObjectList: {
            bool has_hit_anything = false;
            
            f32 closest_so_far = t_max;
            for (u64 obj_index = 0;
                obj_index < obj->obj_list.size;
                ++obj_index) {
                ObjectHandle test_object = object_list_get(&obj->obj_list, obj_index);
              
                HitRecord temp_hit;  
                if (object_hit(world, ray, test_object, t_min, closest_so_far, &temp_hit, data)) {
                    has_hit_anything = true;
                    closest_so_far = temp_hit.t;
                    *hrec = temp_hit;
                }
            }
            
            result = has_hit_anything;
        } break;
        case ObjectType_ConstantMedium: {
            HitRecord hit1, hit2;
            if (object_hit(world, ray, obj->constant_medium.boundary, -INFINITY, INFINITY, &hit1, data)) {
                if (object_hit(world, ray, obj->constant_medium.boundary, hit1.t + 0.0001f, INFINITY, &hit2, data)) {
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
                        f32 hit_dist = obj->constant_medium.neg_inv_density * logf(random(data.entropy));
                        
                        if (hit_dist < distance_inside_boundary) {
                            hrec->t = hit1.t + hit_dist;
                            hrec->p = ray_at(ray, hrec->t);
                            
                            // @NOTE can be not set bacuse not used
                            // hrec->n = v3(1, 0, 0);
                            // hrec->is_front_face = true;
                            hrec->mat = obj->constant_medium.phase_function;
                            
                            result = true;
                        } 
                    }
                }
            }
        } break;
        case ObjectType_Transform: {
            // @TODO something is wrong with hitting instances or bvhs
            Vec3 os_orig = mat4x4_mul_vec3(obj->transform.t.w2o, ray.orig);
            Vec3 os_dir = mat4x4_as_3x3_mul_vec3(obj->transform.t.w2o, ray.dir); 
            Ray os_ray = make_ray(os_orig, os_dir, ray.time);
        
            result = object_hit(world, os_ray, obj->transform.obj, t_min, t_max, hrec, data);
            if (result) {
                Vec3 ws_p = mat4x4_mul_vec3(obj->transform.t.o2w, hrec->p);
                Vec3 ws_n = normalize(mat4x4_as_3x3_mul_vec3(obj->transform.t.o2w, hrec->n));
                
                hrec->p = ws_p;
                hit_set_normal(hrec, ws_n, ray);   
            }
        } break;
        case ObjectType_AnimatedTransform: {
            f32 ray_time = ray.time;
            f32 time = (ray_time - obj->animated_transform.time[0]) / (obj->animated_transform.time[1] - obj->animated_transform.time[0]);
            Vec3 t = v3lerp(obj->animated_transform.t[0], obj->animated_transform.t[1], time);
            Quat4 r = q4lerp(obj->animated_transform.r[0], obj->animated_transform.r[1], time);
            Transform trans = transform_tr(t, r);
            
            Vec3 os_orig = mat4x4_mul_vec3(trans.w2o, ray.orig);
            Vec3 os_dir = mat4x4_as_3x3_mul_vec3(trans.w2o, ray.dir); 
            Ray os_ray = make_ray(os_orig, os_dir, ray.time);
        
            result = object_hit(world, os_ray, obj->animated_transform.obj, t_min, t_max, hrec, data);
            if (result) {
                Vec3 ws_p = mat4x4_mul_vec3(trans.o2w, hrec->p);
                Vec3 ws_n = normalize(mat4x4_as_3x3_mul_vec3(trans.o2w, hrec->n));
                
                hrec->p = ws_p;
                hit_set_normal(hrec, ws_n, ray);   
            }
        } break;
        case ObjectType_BVHNode: {
            if (bounds3_hit(obj->bvh_node.bounds, ray, t_min, t_max)) {
                bool hit_left = object_hit(world, ray, obj->bvh_node.left, t_min, t_max, hrec, data);
                bool hit_right = object_hit(world, ray, obj->bvh_node.right, t_min, hit_left ? hrec->t : t_max, hrec, data);
                result = hit_left || hit_right;
            }
        } break;
        case ObjectType_Box: {
            result = object_hit(world, ray, obj->box.sides_list, t_min, t_max, hrec, data);
        } break;
        case ObjectType_TriangleMesh: {
            if (!bounds3_hit(obj->triangle_mesh.bounds, ray, t_min, t_max)) {
                break;
            }
            
            f32 hit_u, hit_v, hit_t;
            u32 hit_vertex_index = 0;
            
            for (u32 triangle_index = 0;
                 triangle_index < obj->triangle_mesh.ntrig;
                 ++triangle_index) {
                u32 vertex_index = triangle_index * 3;
                Vec3 p0 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index]];        
                Vec3 p1 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index + 1]];        
                Vec3 p2 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index + 2]];        
                f32 t, u, v;
                if (triangle_hit(p0, p1, p2, ray, &t, &u, &v, data.stats)) {
                    if ((t > t_min) && (t < t_max)) {
                        t_max = t;
                        
                        hit_u = u;
                        hit_v = v;
                        hit_t = t;
                        
                        hit_vertex_index = vertex_index;                        
                        result = true;
                    }
                }
            }
            
            if (result) {
                hrec->t = hit_t;
                hrec->p = ray_at(ray, hrec->t);
                
                Vec3 n0 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index]];        
                Vec3 n1 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index + 1]];    
                Vec3 n2 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index + 2]];    
                Vec3 outward_normal = v3add3(v3muls(n0, 1 - hit_u - hit_v), v3muls(n1, hit_u), v3muls(n2, hit_v));
                // @NOTE Problem is that vertices in loaded model are ordered in such way that their normal is pointing outward,
                // so we don't neeed to negate normal
                // hit_set_normal(hrec, outward_normal, ray);
                hrec->n = outward_normal;
                hrec->is_front_face = true;
                Vec2 uv0 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index]];        
                Vec2 uv1 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index + 1]];    
                Vec2 uv2 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index + 2]];    
                Vec2 uv = v2add3(v2muls(uv0, 1 - hit_u - hit_v), v2muls(uv1, hit_u), v2muls(uv2, hit_v));
                hrec->u = uv.x;
                hrec->v = uv.y;
                
                hrec->mat = obj->triangle_mesh.mat;
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    data.stats->object_collision_test_successes += result;
    
    return result;
}

f32 
pdf_value(World *world, PDF pdf, Vec3 dir, RayCastData data) {
    f32 result = 0;
    switch(pdf.type) {
        case PDFType_Cosine: {
            f32 cosine = dot(normalize(dir), pdf.cosine.uvw.w);
            if (cosine > 0) {
                result = cosine / PI;
            } 
        } break;
        case PDFType_Object: {
            result = get_object_pdf_value(world, pdf.object.obj, pdf.object.o, dir, data);
        } break;
        case PDFType_Mixture: {
            result = 0.5f * pdf_value(world, *pdf.mix.p[0], dir, data) +
                     0.5f * pdf_value(world, *pdf.mix.p[1], dir, data);
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
} 

Vec3 
pdf_generate(World *world, PDF pdf, RayCastData data) {
    Vec3 result = {0};
    switch(pdf.type) {
        case PDFType_Cosine: {
            result = onb_local(pdf.cosine.uvw, random_cosine_direction(data.entropy));
        } break;
        case PDFType_Object: {
            result = get_object_random(world, pdf.object.obj, pdf.object.o, data);
        } break;
        case PDFType_Mixture: {
            if (random(data.entropy) < 0.5f) {
                result = pdf_generate(world, *pdf.mix.p[0], data);
            } else {
                result = pdf_generate(world, *pdf.mix.p[1], data);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
ray_cast(World *world, Ray ray, i32 depth, RayCastData data) {
    // Resulting color
    Vec3 color = v3s(0);
    // How much ray contributes to color
    Vec3 throughput = v3s(1.0);
    
    for(u32 bounce = 0;
        bounce < depth;
        ++bounce) {
        ++data.stats->bounce_count;
        
        HitRecord hrec = {0};
        if (!object_hit(world, ray, world->obj_list, 0.001f, INFINITY, &hrec, data)) {
            color = v3add(color, v3mul(throughput, world->backgorund_color));
            break;
        }    
        
        ScatterRecord srec = {0};
        if (!material_scatter(world, ray, hrec, &srec, data)) {
            Vec3 emitted = material_emit(world, ray, hrec, data);
            color = v3add(color, v3mul(throughput, emitted));
            break;
        }
        
        // Specular bounce means that surface does not scatter the ray
        if (srec.is_specular) {
            ray.orig = hrec.p;
            ray.dir = srec.specular_dir;
            throughput = v3mul(throughput, srec.attenuation);
            continue;
        }
        
        PDF pdf;
        PDF ipdf = object_pdf(world->important_objects, hrec.p);
        if (world->has_importance_sampling) {
            pdf = mixture_pdf(&ipdf, &srec.pdf);
        } else {
            pdf = srec.pdf;
        }
        
        ray.orig = hrec.p;
        ray.dir = pdf_generate(world, pdf, data);
        f32 bsdf = material_scattering_pdf(world, ray, hrec);
        f32 pdf_val = pdf_value(world, pdf, ray.dir, data);
        f32 weight = bsdf / pdf_val;
        ray.dir = normalize(ray.dir);
        if (!isnormal(weight) || is_black(srec.attenuation)) {
            break;
        }
        
        throughput = v3mul(throughput, v3muls(srec.attenuation, weight));
#if 1
        // Russian roulette
        if (bounce > 3) {
            f32 p = max32(max32(throughput.x, throughput.y), throughput.z);
            if (random(data.entropy) > min32(p, 0.95f)) {
                ++data.stats->russian_roulette_terminated_bounces;
                break;
            }
            throughput = v3muls(throughput, 1.0f / p);
        }
#endif 
    }
    
    return color;
}
