#include "ray.h"


#include "ray_tracer.c"
#include "ray_math.c"
#include "sys.c"
#include "image.c"

Vec3 ray_point_at(Ray ray, f32 param)
{
    Vec3 result = vec3_add(ray.origin, vec3_muls(ray.dir, param));
    return result;
}

static f32
linear_to_srgb(f32 l)
{
    if (l < 0)
        l = 0;
    if (l > 1)
        l = 1;

    f32 s = l * 12.92f;
    if (l > 0.0031308f)
    {
        s = 1.055f * pow32(l, reciprocal32(2.4f)) - 0.055f;
    }
    return s;
}


const f32 min_hit_distance = 0.001f;
const f32 tolerance = 0.001f;

static inline void 
plane_hit(Plane plane, Ray ray, HitRecord *record)
{
    f32 denominator = vec3_dot(plane.normal, ray.dir);
    if ((denominator < -tolerance) > (denominator > tolerance))
    {
        f32 t = (-plane.dist - vec3_dot(plane.normal, ray.origin)) / denominator;
        if ((t > min_hit_distance) && (t < record->distance))
        {
            record->distance = t;
            record->mat_index = plane.mat_index;
            record->normal = plane.normal;
            record->hit_point = ray_point_at(ray, t);
            record->uv.u = mod32(record->hit_point.x, 1.0f);
            record->uv.v = mod32(record->hit_point.y, 1.0f);
        }
    }
}

static inline void 
sphere_hit(Sphere sphere, Ray ray, HitRecord *record)
{
    Vec3 sphere_relative_ray_origin = vec3_sub(ray.origin, sphere.pos);
    f32 a = vec3_dot(ray.dir, ray.dir);
    f32 b = 2.0f * vec3_dot(sphere_relative_ray_origin, ray.dir);
    f32 c = vec3_dot(sphere_relative_ray_origin, sphere_relative_ray_origin) - sphere.radius * sphere.radius;

    f32 denominator = 2.0f * a;
    f32 root_term = sqrt32(b * b - 4.0f * a * c);

    if (root_term > tolerance)
    {
        f32 tp = (-b + root_term) / denominator;
        f32 tn = (-b - root_term) / denominator;

        f32 t = tp;
        if ((tn > min_hit_distance) && (tn < tp))
        {
            t = tn;
        }

        if ((t > min_hit_distance) && (t < record->distance))
        {
            record->distance = t;
            record->mat_index = sphere.mat_index;
            record->normal = vec3_normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray.dir, record->distance)));
            record->hit_point = ray_point_at(ray, t);
            record->uv = unit_sphere_get_uv(vec3_divs(vec3_sub(record->hit_point, sphere.pos), sphere.radius));
        }
    }
}

static inline void 
moving_sphere_hit(MovingSphere sphere, Ray ray, HitRecord *record)
{
    Vec3 sphere_pos = moving_sphere_center(&sphere, ray.time);
    Vec3 sphere_relative_ray_origin = vec3_sub(ray.origin, sphere_pos);
    f32 a = vec3_dot(ray.dir, ray.dir);
    f32 b = 2.0f * vec3_dot(sphere_relative_ray_origin, ray.dir);
    f32 c = vec3_dot(sphere_relative_ray_origin, sphere_relative_ray_origin) - sphere.radius * sphere.radius;

    f32 denominator = 2.0f * a;
    f32 root_term = sqrt32(b * b - 4.0f * a * c);

    if (root_term > tolerance)
    {
        f32 tp = (-b + root_term) / denominator;
        f32 tn = (-b - root_term) / denominator;

        f32 t = tp;
        if ((tn > min_hit_distance) && (tn < tp))
        {
            t = tn;
        }

        if ((t > min_hit_distance) && (t < record->distance))
        {
            record->distance = t;
            record->mat_index = sphere.mat_index;
            record->normal = vec3_normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray.dir, record->distance)));
            record->hit_point = ray_point_at(ray, t);
            record->uv = unit_sphere_get_uv(vec3_divs(vec3_sub(record->hit_point, sphere_pos), sphere.radius));
        }
    }
}

static bool 
ray_hit_aabb(AABB aabb, Ray ray, f32 tmin, f32 tmax)
{
    bool result = true;
    
    for(u32 i = 0;
        i < 3;
        ++i)
    {
        f32 rd = reciprocal32(ray.dir.e[i]);
        f32 t0 = (aabb.min.e[i] - ray.origin.e[i]) * rd; 
        f32 t1 = (aabb.max.e[i] - ray.origin.e[i]) * rd; 
        if (rd < 0.0f)
        {
            swap(t0, t1);
        }

        tmin = max(tmin, t0);
        tmax = min(tmax, t1);        
        if (tmax <= tmin)
        {
            result = false;
            break;
        }
    }
    
    return result;
}

void 
hitable_hit(Hitable *hitable, Ray ray, HitRecord *record)
{
    switch(hitable->type)
    {
        default: assert(false); break;
        case Hitable_Sphere:
        {
            sphere_hit(hitable->sphere, ray, record);
        } break;
        case Hitable_MovingSphere:
        {
            moving_sphere_hit(hitable->moving_sphere, ray, record);
        } break;
        case Hitable_BVHNode:
        {
            
        } break;
    }
}


static AABB
sphere_bounding_box(Sphere sphere, f32 t0, f32 t1)
{
    AABB result = { 
        .min = vec3_sub(sphere.pos, vec3s(sphere.radius)),  
        .max = vec3_add(sphere.pos, vec3s(sphere.radius)),  
    };
    
    return result;
}

static AABB
moving_sphere_bounding_box(MovingSphere sphere, f32 t0, f32 t1)
{
    AABB box0 = { 
        .min = vec3_sub(sphere.center0, vec3s(sphere.radius)),  
        .max = vec3_add(sphere.center0, vec3s(sphere.radius)),  
    };
    AABB box1 = { 
        .min = vec3_sub(sphere.center1, vec3s(sphere.radius)),  
        .max = vec3_add(sphere.center1, vec3s(sphere.radius)),  
    };
    
    AABB result = aabb_join(box0, box1);
    
    return result;
}

Vec3 
ray_surface_normal(Ray ray, Vec3 normal)
{
    Vec3 result = normal;
    if (!(vec3_dot(ray.dir, normal) < 0))
    {
        result = vec3_neg(normal);
    }    
    return result;
}

static void
cast_sample_rays(CastState *state)
{
    Scene *scene = state->scene;
    Camera *camera = &scene->camera;
    u32 rays_per_pixel = state->rays_per_pixel;
    u32 max_bounce_count = state->max_bounce_count;
    RandomSeries series = state->series;
    f32 film_x = state->film_x;
    f32 film_y = state->film_y;
    Vec3 final_color = vec3(0, 0, 0);
    u64 bounces_computed = 0;

    u32 lane_width = 1;
    u32 lane_ray_count = rays_per_pixel / lane_width;
    f32 contrib = reciprocal32(lane_ray_count);
    for (u32 ray_index = 0;
         ray_index < lane_ray_count;
         ++ray_index)
    {
        Ray ray = camera_ray_at(camera, film_x, film_y, &series);
        
        Vec3 ray_cast_color = vec3(0, 0, 0);
        Vec3 attenuation = vec3(1, 1, 1);

        for (u32 bounce_count = 0;
             bounce_count < max_bounce_count;
             ++bounce_count)
        {
            ++bounces_computed;
            HitRecord hit_record = {0};
            hit_record.distance = F32_MAX;
            
            // Iterate scene objects

            for (u32 plane_index = 0;
                 plane_index < scene->plane_count;
                 ++plane_index)
            {
                Plane plane = scene->planes[plane_index];
                plane_hit(plane, ray, &hit_record);
            }

            for (u32 sphere_index = 0;
                 sphere_index < scene->sphere_count;
                 ++sphere_index)
            {
                Sphere sphere = scene->spheres[sphere_index];
                sphere_hit(sphere, ray, &hit_record);
            }
            
            for (u32 moving_sphere_index = 0;
                 moving_sphere_index < scene->moving_sphere_count;
                 ++moving_sphere_index)
            {
                MovingSphere sphere = scene->moving_spheres[moving_sphere_index];
                moving_sphere_hit(sphere, ray, &hit_record);
            }
            
            for (u32 rect_index = 0;
                 rect_index < scene->rect_count;
                 ++rect_index)
            {
                Rect rect_ = scene->rects[rect_index];
                
                f32 sin_theta = sin32(rect_.rotation_y);
                f32 cos_theta = cos32(rect_.rotation_y);
                
                Ray rotated_ray = ray;
                rotated_ray.origin.x = cos_theta * ray.origin.x - sin_theta * ray.origin.y;
                rotated_ray.origin.y = sin_theta * ray.origin.x + cos_theta * ray.origin.y;
                
                rotated_ray.dir.x = cos_theta * ray.dir.x - sin_theta * ray.dir.y;
                rotated_ray.dir.y = sin_theta * ray.dir.x + cos_theta * ray.dir.y;
                
                bool made_hit = false;
                switch(rect_.type)
                {
                    case RectType_XY:
                    {
                        XYRect rect = rect_.xy;
                        f32 t = (rect.k - ray.origin.z) / ray.dir.z;
                        if ((t > min_hit_distance) && (t < hit_record.distance))
                        {
                            f32 x = rotated_ray.origin.x + t * rotated_ray.dir.x;
                            f32 y = rotated_ray.origin.y + t * rotated_ray.dir.y;
                            if ((rect.x0 < x) && (x < rect.x1) && 
                                (rect.y0 < y) && (y < rect.y1))
                            {
                                made_hit = true;
                                hit_record.distance = t;
                                hit_record.mat_index = rect.mat_index;
                                hit_record.normal = ray_surface_normal(rotated_ray, vec3(0, 0, 1));
                                // hit_record.normal = vec3(0, 0, 1);
                                hit_record.hit_point = ray_point_at(rotated_ray, t);
                                hit_record.uv.u = (x - rect.x0) / (rect.x1 - rect.x0);
                                hit_record.uv.v = (y - rect.y0) / (rect.y1 - rect.y0);
                            }
                        }
                    } break;
                    case RectType_XZ:
                    {
                        XZRect rect = rect_.xz;
                        f32 t = (rect.k - rotated_ray.origin.y) / rotated_ray.dir.y;
                        if ((t > min_hit_distance) && (t < hit_record.distance))
                        {
                            f32 x = rotated_ray.origin.x + t * rotated_ray.dir.x;
                            f32 z = rotated_ray.origin.z + t * rotated_ray.dir.z;
                            if ((rect.x0 < x) && (x < rect.x1) && 
                                (rect.z0 < z) && (z < rect.z1))
                            {
                                made_hit = true;
                                hit_record.distance = t;
                                hit_record.mat_index = rect.mat_index;
                                hit_record.normal = ray_surface_normal(rotated_ray, vec3(0, 1, 0));
                                // hit_record.normal = vec3(0, 1, 0);
                                hit_record.hit_point = ray_point_at(rotated_ray, t);
                                hit_record.uv.u = (x - rect.x0) / (rect.x1 - rect.x0);
                                hit_record.uv.v = (z - rect.z0) / (rect.z1 - rect.z0);
                            }
                        }
                    } break;
                    case RectType_YZ:
                    {
                        YZRect rect = rect_.yz;
                        f32 t = (rect.k - rotated_ray.origin.x) / rotated_ray.dir.x;
                        if ((t > min_hit_distance) && (t < hit_record.distance))
                        {
                            f32 y = rotated_ray.origin.y + t * rotated_ray.dir.y;
                            f32 z = rotated_ray.origin.z + t * rotated_ray.dir.z;
                            if ((rect.y0 < y) && (y < rect.y1) && 
                                (rect.z0 < z) && (z < rect.z1))
                            {
                                made_hit = true;
                                hit_record.distance = t;
                                hit_record.mat_index = rect.mat_index;
                                hit_record.normal = ray_surface_normal(rotated_ray, vec3(1, 0, 0));
                                // hit_record.normal = vec3(1, 0, 0);
                                hit_record.hit_point = ray_point_at(rotated_ray, t);
                                hit_record.uv.u = (y - rect.y0) / (rect.y1 - rect.y0);
                                hit_record.uv.v = (z - rect.z0) / (rect.z1 - rect.z0);
                            }
                        }
                    } break;
                }
                
                if (made_hit)
                {
                    Vec3 hit_point = hit_record.hit_point;
                    Vec3 normal = hit_record.normal;
                    
                    hit_record.hit_point.x =  cos_theta * hit_point.x + sin_theta * hit_point.y;
                    hit_record.hit_point.y = -sin_theta * hit_point.x + cos_theta * hit_point.y;
                    
                    normal.x =  cos_theta * normal.x + sin_theta * normal.y;
                    normal.y = -sin_theta * normal.x + cos_theta * normal.y;
                    
                    hit_record.normal = ray_surface_normal(rotated_ray, normal);
                }
            }

            if (has_hit(hit_record))
            {
                Material mat = scene->materials[hit_record.mat_index];

                ray.origin = hit_record.hit_point;

                // Decide if we want to refract or reflect
                // Check if refraction_probability != 0 (it is OK to compare floats against constants)
                if ((mat.refraction_probability != 0.0f)) //&& (random_unitlateral(&series) <= mat.refraction_probability))
                {
                    Vec3 reflected = vec3_reflect(ray.dir, hit_record.normal);
                    // Coefficient in Snells law between glass and air
                    f32 ref_idx = 1.5f;
                    // sin(Theta0) / sin(Theta1) - different when light goes in or out of glass
                    f32 refract_coef;
                    f32 cos_atten;
                    Vec3 actual_normal;
                    if (vec3_dot(ray.dir, hit_record.normal) > 0)
                    {
                        refract_coef = ref_idx;
                        cos_atten = ref_idx * vec3_dot(ray.dir, hit_record.normal);
                        actual_normal = vec3_neg(hit_record.normal);
                    }
                    else
                    {
                        actual_normal = hit_record.normal;
                        refract_coef = 1.0f / ref_idx;
                        cos_atten = -vec3_dot(ray.dir, hit_record.normal);
                    }

                    f32 reflect_prob;
                    f32 dt = vec3_dot(ray.dir, actual_normal);
                    f32 discriminant = 1.0f - refract_coef * refract_coef * (1.0f - dt * dt);
                    Vec3 refracted;

                    if (discriminant > 0.0f)
                    {
                        refracted = vec3_sub(vec3_muls(vec3_sub(ray.dir, vec3_muls(actual_normal, dt)), refract_coef), vec3_muls(actual_normal, sqrt32(discriminant)));
                        reflect_prob = schlick(cos_atten, ref_idx);
                    }
                    else
                    {
                        reflect_prob = 1.0f;
                    }

                    if (random_unitlateral(&series) <= reflect_prob)
                    {
                        ray.dir = vec3_normalize(reflected);
                    }
                    else
                    {
                        ray.dir = vec3_normalize(refracted);
                    }
                }
                else
                {
                    ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));
                    f32 cos_atten = vec3_dot(vec3_neg(ray.dir), hit_record.normal);
                    if (cos_atten < 0.0f)
                    {
                        cos_atten = 0.0f;
                    }
                    Vec3 reflect_value = { 0 };
                    if (mat.texture.proc)
                    {
                        reflect_value = mat.texture.proc(&mat.texture, hit_record.uv, hit_record.hit_point);
                    }
                    attenuation = vec3_mul(attenuation, vec3_muls(reflect_value, cos_atten));

                    Vec3 pure_reflection = vec3_reflect(ray.dir, hit_record.normal);
                    Vec3 scattered_reflection = vec3_normalize(vec3_add(hit_record.normal, random_unit_sphere(&series)));
                    ray.dir = vec3_normalize(vec3_lerp(scattered_reflection, pure_reflection, mat.scatter));
                }
            }
            else
            {
                Material mat = scene->materials[hit_record.mat_index];
                ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));
                break;
            }
        }

        final_color = vec3_add(final_color, vec3_muls(ray_cast_color, contrib));
    }

    final_color = vec3_muls(vec3(linear_to_srgb(final_color.x),
                                 linear_to_srgb(final_color.y),
                                 linear_to_srgb(final_color.z)),
                            255.0f);
    state->bounces_computed += bounces_computed;
    state->final_color = final_color;
    state->series = series;
}

// Gets work order from queue and checks if it is present.
// If no work needs to be done (all tiles are finished) returns false and thus makes thread exit (look at render_thread_proc)
// Otherwise sets up rendering settings
static bool
render_tile(RenderWorkQueue *queue)
{
    u64 work_order_index = atomic_add64(&queue->next_work_order_index, 1);
    if (work_order_index >= queue->work_order_count)
    {
        return false;
    }

    CastState state = {0};
    RenderWorkOrder *order = queue->work_orders + work_order_index;

    state.scene = order->scene;
    state.series = order->random_series;
    // @TODO(hl): Settings for these fellas
    state.rays_per_pixel = 1024;
    state.max_bounce_count = 8;

    ImageU32 *image = order->image;

    u32 xmin = order->xmin;
    u32 ymin = order->ymin;
    u32 one_past_xmax = order->one_past_xmax;
    u32 one_past_ymax = order->one_past_ymax;
    for (u32 y = ymin;
         y < one_past_ymax;
         ++y)
    {
        u32 *out = image_u32_get_pixel_pointer(image, xmin, y);
        state.film_y = ((f32)y / (f32)image->height) * 2.0f - 1.0f;

        for (u32 x = xmin;
             x < one_past_xmax;
             ++x)
        {
            state.film_x = ((f32)x / (f32)image->width) * 2.0f - 1.0f;

            cast_sample_rays(&state);
            Vec3 color = state.final_color;

            u32 out_value = rgba_pack_4x8(vec4(color.x, color.y, color.z, 255.0f));
            *out++ = out_value;
        }
    }

    atomic_add64(&queue->bounces_computed, state.bounces_computed);
    atomic_add64(&queue->tile_retired_count, 1);

    return true;
}

// Work procedure of all tile rendering threads
static THREAD_PROC_SIGNATURE(render_thread_proc)
{
    RenderWorkQueue *queue = param;

    while (render_tile(queue))
    {
    }

    sys_exit_thread();
    return 0;
}

// Returns scene with same objects in it
// Can be used for testing and comparing results
void init_sample_scene(Scene *scene, ImageU32 *image)
{
    // @NOTE(hl): Don't malloc arrays because scene settings is always the same
    static Material materials[8] = {0};
    materials[0].emit_color = vec3(0.3f, 0.4f, 0.5f);
    // materials[1].texture = texture_solid_color(vec3(0.5f, 0.5f, 0.5f));
    materials[1].texture = texture_checkered(vec3(0.5, 0.5, 0.5), vec3(0.2, 0.3, 0.1));
    materials[2].texture = texture_solid_color(vec3(0.7f, 0.5f, 0.3f));
    materials[2].refraction_probability = 1.0f;
    materials[3].emit_color = vec3(15.0f, 15.0f, 15.0f);
    // materials[4].texture = texture_solid_color(vec3(0.2f, 0.8f, 0.2f));
    // materials[4].scatter = 0.7f;
    materials[4].texture = texture_image("e:\\dev\\ray\\data\\earthmap.jpg");
    materials[5].texture = texture_solid_color(vec3(0.4f, 0.8f, 0.9f));
    materials[5].scatter = 0.85f;
    materials[6].texture = texture_solid_color(vec3(0.95f, 0.95f, 0.95f));
    materials[6].scatter = 0.99f;
    materials[7].texture = texture_solid_color(vec3(0.75f, 0.33f, 0.75f));

    // static Plane planes[1] = {0};
    // planes[0].mat_index = 1;
    // planes[0].normal = vec3(0, 0, 1);
    // planes[0].dist = 0;

    static Sphere spheres[5] = {0};
    spheres[0].pos = vec3(-3, 2, 0);
    spheres[0].radius = 2.0f;
    spheres[0].mat_index = 6;

    spheres[1].pos = vec3(3, -2, 0);
    spheres[1].radius = 1.0f;
    spheres[1].mat_index = 3;

    spheres[2].pos = vec3(-2, -1, 2);
    spheres[2].radius = 1.0f;
    spheres[2].mat_index = 4;

    spheres[3].pos = vec3(1, -1, 3);
    spheres[3].radius = 1.0f;
    spheres[3].mat_index = 5;

    spheres[4].pos = vec3(1, -3, 1);
    spheres[4].radius = 1.0f;
    spheres[4].mat_index = 2;
    
    // spheres[5].pos = vec3(0, 0, -1000);
    // spheres[5].radius = 1000.0f;
    // spheres[5].mat_index = 1;
    
    static MovingSphere moving_spheres[1] = { 0 };
    moving_spheres[0].center0 = vec3(-2, -3, 0);
    moving_spheres[0].center1 = vec3(-2, -3, 1);
    moving_spheres[0].mat_index = 7;
    moving_spheres[0].radius = 0.5f;
    moving_spheres[0].time0 = 0;    
    moving_spheres[0].time1 = 1;    
    
    static Rect rects[1] = { 0 };
    rects[0] = (Rect) { 
        .type = RectType_XY,
        .xy   = (XYRect) {
            .x0 = -555,
            .y0 = -555,
            .x1 = 555,
            .y1 = 555,
            .k = 0,
            .mat_index = 1 
        }
    };
    scene->rect_count = array_size(rects);
    scene->rects = rects;
    
    Camera camera;
    camera.time0 = 0;
    camera.time1 = 1;
    // Camera is an orthographic projection with film having size of dest image
    camera.camera_pos = vec3(-1.5, -10, 1.5);
    camera.camera_pos = vec3_muls(camera.camera_pos, 1.5f);
    // @NOTE(hl): Create coordinate system for camera.
    // These are unit vectors of 3 axes of our coordinate system
    camera.camera_z = vec3_normalize(camera.camera_pos);
    camera.camera_x = vec3_normalize(vec3_cross(vec3(0, 0, 1), camera.camera_z));
    camera.camera_y = vec3_normalize(vec3_cross(camera.camera_z, camera.camera_x));
    camera.film_center = vec3_sub(camera.camera_pos, camera.camera_z);

    // Apply image aspect ratio
    camera.film_w = 1.0f;
    camera.film_h = 1.0f;
    if (image->width > image->height)
    {
        camera.film_h = camera.film_w * (f32)image->height / (f32)image->width;
    }
    else if (image->height > image->width)
    {
        camera.film_w = camera.film_h * (f32)image->width / (f32)image->height;
    }
    camera.half_film_w = camera.film_w * 0.5f;
    camera.half_film_h = camera.film_h * 0.5f;
    camera.half_pix_w = reciprocal32(image->width) * 0.5f;
    camera.half_pix_h = reciprocal32(image->height) * 0.5f;
    scene->camera = camera;

    scene->material_count = array_size(materials);
    scene->materials = materials;
    scene->sphere_count = array_size(spheres);
    scene->spheres = spheres;
    // scene->plane_count = array_size(planes);
    // scene->planes = planes;
    scene->moving_sphere_count = array_size(moving_spheres);
    scene->moving_spheres = moving_spheres;
}

void 
add_box(Rect *rects, AABB box, u32 mat_index, f32 rotation)
{
    *rects++ = (Rect) {
        .type = RectType_XY,
        .rotation_y = rotation,
        .xy   = (XYRect) {
            .x0 = box.min.x,
            .y0 = box.min.y,
            .x1 = box.max.x,
            .y1 = box.max.y,
            .k  = box.min.z,
            .mat_index = mat_index
        },
    };
    *rects++ = (Rect) {
        .type = RectType_XY,
        .rotation_y = rotation,
        .xy   = (XYRect) {
            .x0 = box.min.x,
            .y0 = box.min.y,
            .x1 = box.max.x,
            .y1 = box.max.y,
            .k  = box.max.z,
            .mat_index = mat_index
        }
    };
    *rects++ = (Rect) {
        .type = RectType_YZ,
        .rotation_y = rotation,
        .yz   = (YZRect) {
            .y0 = box.min.y,
            .z0 = box.min.z,
            .y1 = box.max.y,
            .z1 = box.max.z,
            .k  = box.min.x,
            .mat_index = mat_index
        }
    };
    *rects++ = (Rect) {
        .type = RectType_YZ,
        .rotation_y = rotation,
        .yz   = (YZRect) {
            .y0 = box.min.y,
            .z0 = box.min.z,
            .y1 = box.max.y,
            .z1 = box.max.z,
            .k  = box.max.x,
            .mat_index = mat_index
        }
    };
    *rects++ = (Rect) {
        .type = RectType_XZ,
        .rotation_y = rotation,
        .xz   = (XZRect) {
            .x0 = box.min.x,
            .z0 = box.min.z,
            .x1 = box.max.x,
            .z1 = box.max.z,
            .k  = box.min.y,
            .mat_index = mat_index
        }
    };
    *rects++ = (Rect) {
        .type = RectType_XZ,
        .rotation_y = rotation,
        .xz   = (XZRect) {
            .x0 = box.min.x,
            .z0 = box.min.z,
            .x1 = box.max.x,
            .z1 = box.max.z,
            .k  = box.max.y,
            .mat_index = mat_index
        }
    };
}

void 
make_colonel_box(Scene *scene, ImageU32 *image)
{
    static Material materials[7] = { 0 };
    materials[1].texture = texture_solid_color(vec3(0.65f, 0.05f, 0.05f));
    materials[1].scatter = 0.5f;
    materials[2].texture = texture_solid_color(vec3(0.73f, 0.73f, 0.73f));
    materials[3].texture = texture_solid_color(vec3(0.12f, 0.45f, 0.15f));
    materials[3].scatter = 0.5f;
    materials[4].emit_color = vec3(25, 25, 25);
    materials[5].texture = texture_image("e:\\dev\\ray\\data\\earthmap.jpg");
    materials[6].texture = texture_solid_color(vec3(0.7f, 0.5f, 0.3f));
    materials[6].refraction_probability = 1.0f;

    
    static Rect rects[12] = { 0 };
    rects[0] = (Rect) { 
        .type = RectType_YZ,
        .yz   = (YZRect) {
            .y0 = -5,
            .z0 = -5,
            .y1 = 5,
            .z1 = 5,
            .k = -5,
            .mat_index = 1
        }
    };
    rects[1] = (Rect) { 
        .type = RectType_YZ,
        .yz   = (YZRect) {
            .y0 = -5,
            .z0 = -5,
            .y1 = 5,
            .z1 = 5,
            .k = 5,
            .mat_index = 3
        }
    };
    rects[2] = (Rect) { 
        .type = RectType_XY,
        .xy = (XYRect) {
            .x0 = -1.2f,
            .y0 = -1.2f,
            .x1 = 1.2f,
            .y1 = 1.2f,
            .k = 4.99f,
            .mat_index = 4
        }
    };
    rects[3] = (Rect) { 
        .type = RectType_XY,
        .xy   = (XYRect) {
            .x0 = -5,
            .y0 = -5,
            .x1 = 5,
            .y1 = 5,
            .k = 5,
            .mat_index = 2
        }
    };
    rects[4] = (Rect) { 
        .type = RectType_XY,
        .xy   = (XYRect) {
            .x0 = -5,
            .y0 = -5,
            .x1 = 5,
            .y1 = 5,
            .k = -5,
            .mat_index = 2 
        }
    };
    rects[5] = (Rect) { 
        .type = RectType_XZ,
        .xz   = (XZRect) {
            .x0 = -5,
            .z0 = -5,
            .x1 = 5,
            .z1 = 5,
            .k = 5,
            .mat_index = 2 
        }
    };
    
    static Sphere spheres[1] = { 0 };
    spheres[0] = (Sphere) {
        .mat_index = 6,
        .pos = vec3(2.1f, -2.3f, -3.5f),
        .radius = 1.5f
    };
    // spheres[1] = (Sphere) {
    //     .mat_index = 6,
    //     .pos = vec3(-1.25f, 2.0f, -3.0f),
    //     .radius = 2.0f
    // };
    scene->sphere_count = array_size(spheres);
    scene->spheres = spheres;
    
    // AABB box0 = { 
    //     .min = vec3(0.7f, -3.8f, -5.0f),
    //     .max = vec3(3.5f, -0.9f, -2.0f)
    // };
    // add_box(rects + 6, box0, 2,  0.261799f * 1.3f);
    AABB box1 = {
        .min = vec3(-2.75f, 0.3f, -5.0f),
        .max = vec3(  0.2f, 3.3f,  0.95f)
    };
    add_box(rects + 6, box1, 2, -0.314159265f);
     
    Camera camera;
    camera.time0 = 0;
    camera.time1 = 1;
    // Camera is an orthographic projection with film having size of dest image
    camera.camera_pos = vec3(0, -16, 0);
    // @NOTE(hl): Create coordinate system for camera.
    // These are unit vectors of 3 axes of our coordinate system
    camera.camera_z = vec3_normalize(camera.camera_pos);
    camera.camera_x = vec3_normalize(vec3_cross(vec3(0, 0, 1), camera.camera_z));
    camera.camera_y = vec3_normalize(vec3_cross(camera.camera_z, camera.camera_x));
    camera.film_center = vec3_sub(camera.camera_pos, camera.camera_z);

    // Apply image aspect ratio
    camera.film_w = 1.0f;
    camera.film_h = 1.0f;
    if (image->width > image->height)
    {
        camera.film_h = camera.film_w * (f32)image->height / (f32)image->width;
    }
    else if (image->height > image->width)
    {
        camera.film_w = camera.film_h * (f32)image->width / (f32)image->height;
    }
    camera.half_film_w = camera.film_w * 0.5f;
    camera.half_film_h = camera.film_h * 0.5f;
    camera.half_pix_w = reciprocal32(image->width) * 0.5f;
    camera.half_pix_h = reciprocal32(image->height) * 0.5f;
    scene->camera = camera;

    scene->material_count = array_size(materials);
    scene->materials = materials;
    scene->rect_count = array_size(rects);
    scene->rects = rects;
}

int main(int argc, char **argv)
{
    printf("Ray started!\n");
    
    // Raycasting output is simply an image
    ImageU32 image = {0};
    image_u32_init(&image, 1500, 1500);

    // Initialize scene
    Scene scene = {0};
    // init_sample_scene(&scene, &image);
    make_colonel_box(&scene, &image);

    // Record time spend on raycasting
    clock_t start_clock = clock();

    // Get core count to initialize threads
    u32 core_count = sys_get_processor_count();
    // Decide how image is divided in tiles that will be given to threads
    u32 tile_width = 64, tile_height = 64;
    // Calculate tile count
    u32 tile_count_x = (image.width + tile_width - 1) / tile_width;
    u32 tile_count_y = (image.height + tile_height - 1) / tile_height;
    u32 total_tile_count = tile_count_x * tile_count_y;

    printf("Core count: %d, tile size: %u %ux%u (%lluk/tile)\n", core_count, total_tile_count, tile_width, tile_height,
           (tile_width * tile_height * sizeof(u32)) / 1024);

    // Initialize multithreaded work orders for rendering
    // Because all tiles respond for different pixel groups with no intersections, we can have
    // lockless write access to image. And since we also only read from scene structure,
    // we have no locks at all.
    RenderWorkQueue queue = {0};
    queue.work_orders = malloc(sizeof(RenderWorkOrder) * total_tile_count);

    // Iterate tiles and find their bounds
    for (u32 tile_y = 0;
         tile_y < tile_count_y;
         ++tile_y)
    {
        u32 ymin = tile_y * tile_height;
        u32 one_past_ymax = ymin + tile_height;
        if (one_past_ymax > image.height)
        {
            one_past_ymax = image.height;
        }

        for (u32 tile_x = 0;
             tile_x < tile_count_x;
             ++tile_x)
        {
            u32 xmin = tile_x * tile_width;
            u32 one_past_xmax = xmin + tile_width;
            if (one_past_xmax > image.width)
            {
                one_past_xmax = image.width;
            }

            RenderWorkOrder *order = queue.work_orders + queue.work_order_count++;
            assert(queue.work_order_count <= total_tile_count);
            // Record data to order
            order->scene = &scene;
            order->image = &image;
            order->xmin = xmin;
            order->ymin = ymin;
            order->one_past_xmax = one_past_xmax;
            order->one_past_ymax = one_past_ymax;
            // Set random series of each tile to be dependent on tile position, just to make images with same settings
            // have same 'random' values
            order->random_series = (RandomSeries){tile_x * 12098 + tile_y * 23771};
        }
    }

    assert(queue.work_order_count == total_tile_count);

    atomic_add64(&queue.next_work_order_index, 0);

    // Create threads starting from 1 (0 is main thread).
    // Do not store threads anywhere, they will exit themselves when work is done
    for (u32 core_index = 1;
         core_index < core_count;
         ++core_index)
    {
        sys_create_thread(render_thread_proc, &queue);
    }

    // Main thread work
    while (queue.tile_retired_count < total_tile_count)
    {
        if (render_tile(&queue))
        {
            // Simple status alert
            f32 percent = (f32)queue.tile_retired_count / (f32)total_tile_count;
            // f32 time_passed = (f32)(clock() - start_clock);
            // f32 time_left   = time_passed * (1.0f / percent - 1);
            printf("\rRaycasting %d%%", round32(100 * percent));
            fflush(stdout);
        }
    }

    // Calculate time elapsed
    clock_t end_clock = clock();
    clock_t time_elapsed = end_clock - start_clock;

    printf("\n");
    printf("Raycasting time: %lldms\n", (i64)time_elapsed);
    printf("Total bounces: %llu\n", queue.bounces_computed);
    printf("Perfomance: %fms/bounce", (f64)time_elapsed / (f64)queue.bounces_computed);

    // Save image
    char *filename = "out.png";
    image_u32_save(&image, filename);

    printf("\nRay finished!\n");
    return 0;
}