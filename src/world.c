#include "world.h"
#include "trace.h"

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

void 
world_init(World *world) {
    memset(world, 0, sizeof(*world));
    
    world->arena.data_capacity = MEGABYTES(32);
    world->arena.data = malloc(world->arena.data_capacity);
    
    world->obj_list = object_list(world);
    world->important_objects = object_list(world);
}

TextureHandle 
new_texture(World *world, Texture texture) {
    assert(texture.type);
    EXPAND_IF_NEEDED(&world->arena, world->textures, world->textures_size, world->textures_capacity);
    
    TextureHandle handle = { world->textures_size };
    world->textures[world->textures_size++] = texture;
    
    return handle;
}

MaterialHandle 
new_material(World *world, Material material) {
    assert(material.type);
    EXPAND_IF_NEEDED(&world->arena, world->materials, world->materials_size, world->materials_capacity);
    
    MaterialHandle handle = { world->materials_size };
    world->materials[world->materials_size++] = material;
    
    return handle;
}

static void 
has_enough_object_space_or_expand(World *world, u32 requested) {
    if (world->objects_size + requested > world->objects_capacity) {
        world->objects = arena_realloc(&world->arena, world->objects, world->objects_capacity * sizeof(Object),
                                       (world->objects_size + requested) * sizeof(Object));
        world->objects_capacity = world->objects_size + requested;
    }
}

ObjectHandle 
new_object(World *world, Object obj) {
    assert(obj.type);
    has_enough_object_space_or_expand(world, 1);
    // EXPAND_IF_NEEDED(&world->arena, world->objects, world->objects_size, world->objects_capacity);
    
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
material_lambertian(World *world, TextureHandle diffuse) {
    Material material;
    material.type = MaterialType_Lambertian;
    material.diffuse = diffuse;
    
    return new_material(world, material);    
}

MaterialHandle 
material_isotropic(World *world, TextureHandle albedo) {
    Material material;
    material.type = MaterialType_Isotropic;
    // material.isotropic.albedo = albedo;
    
    return new_material(world, material);    
}


MaterialHandle 
material_metal(World *world, f32 roughness, TextureHandle specular) {
    assert(0 <= roughness && roughness <= 1.0);
    
    Material material;
    material.type = MaterialType_Metal;
    material.specular = specular;
    material.roughness = roughness;
    
    return new_material(world, material);    
}

MaterialHandle 
material_plastic(World *world, f32 roughness,
                 f32 ext_ior, f32 int_ior_eta, TextureHandle diffuse, TextureHandle specular) {
    assert(0 <= roughness && roughness <= 1.0);
    
    Material material;
    material.type = MaterialType_Plastic;
    material.roughness = roughness;
    material.specular = specular;
    material.diffuse = diffuse;
    material.int_ior = int_ior_eta;
    material.ext_ior = ext_ior;
    
    return new_material(world, material);                     
}

MaterialHandle 
material_dielectric(World *world, f32 roughness, f32 ext_ior, f32 int_ior, TextureHandle specular, TextureHandle transmittance) {
    Material material;
    material.type = MaterialType_Dielectric;
    material.roughness = roughness;
    material.ext_ior = ext_ior;
    material.int_ior = int_ior;
    material.specular = specular;
    material.transmittance = transmittance;
    
    return new_material(world, material);    
}

MaterialHandle 
material_diffuse_light(World *world, TextureHandle t, LightFlags flags) {
    Material material;
    material.type = MaterialType_DiffuseLight;
    material.emittance = t;
    material.light_flags = flags;
    
    return new_material(world, material);    
}

MaterialHandle 
material_mirror(World *world) {
    Material material;
    material.type = MaterialType_Mirror;
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
object_bvh_node(World *world, ObjectHandle *objs_init, i64 n) {
    Object obj;
    obj.type = ObjectType_BVHNode;
    
    has_enough_object_space_or_expand(world, n);
    
    TempMemory temp_mem = temp_memory_begin(&world->arena);
    Bounds3 *objs_bounds = arena_alloc(&world->arena, sizeof(Bounds3) * n); 
    f32 *left_area  = arena_alloc(&world->arena, sizeof(f32) * n); 
    f32 *right_area = arena_alloc(&world->arena, sizeof(f32) * n); 
    ObjectHandle *objs = arena_copy(&world->arena, objs_init, n * sizeof(ObjectHandle));
    
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
    
    // @NOTE something is very wrong with our allocation strategy here so it causes crash
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

bool 
validate_world(World *world) {
    bool result = true;
    
    for (u32 object_index = 0;
         object_index < world->objects_size;
         ++object_index) {
        Object *object = world->objects + object_index;
        assert(object->type);        
    }
    
    return result;    
}
