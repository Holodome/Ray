#include "Ray.h"

#include "Thirdparty/stb_image_write.h"

#include "RayMath.c"
#include "Sys.c"

Vec3
ray_point_at(Ray ray, f32 param)
{
    Vec3 result = vec3_add(ray.origin, vec3_muls(ray.dir, param));
    return result;
}

ImageU32
image_u32_make(u32 width, u32 height)
{
    ImageU32 result;

    result.width = width;
    result.height = height;
    result.pixels = malloc(sizeof(u32) * width * height);

    return result;
}

u32 *
image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y)
{
    u32 stride = image->width;
    u32 *result = image->pixels + stride * y + x;
    return result;
}

void
image_u32_save(ImageU32 *image, char *filename)
{
    stbi_flip_vertically_on_write(true);
    bool saved = stbi_write_png(filename, image->width, image->height, 4, image->pixels, sizeof(u32) * image->width);
    if (!saved)
    {
        fprintf(stderr, "[ERROR] Failed to save image to '%s'\n", filename);
    }
}

static f32
linear_to_srgb(f32 l)
{
    if (l < 0) l = 0;
    if (l > 1) l = 1;

    f32 s =  l * 12.92f;
    if (l > 0.0031308f)
    {
        s = 1.055f * pow32(l, reciprocal32(2.4f)) - 0.055f;
    }
    return s;
}

static bool
render_tile(RenderWorkQueue *queue)
{
    u32 rays_per_pixel = 32;

    u32 work_order_index = atomic_add64(&queue->next_work_order_index, 1);
    if (work_order_index >= queue->work_order_count)
    {
        return false;
    }

    RenderWorkOrder *order = queue->work_orders + work_order_index;
    RandomSeries series = order->random_series;

    World *world = order->world;
    ImageU32 *image = order->image;
    u32 xmin = order->xmin;
    u32 ymin = order->ymin;
    u32 one_past_xmax = order->one_past_xmax;
    u32 one_past_ymax = order->one_past_ymax;
    // @TODO(hl): Pass as world
    Vec3 camera_pos = vec3(0, -20, 11);
    // @NOTE(hl): Create coordinate system from camera.
    // These are unit vectors of 3 axes of our coordinate system
    Vec3 camera_z = vec3_normalize(camera_pos);
    Vec3 camera_x = vec3_normalize(vec3_cross(vec3(0, 0, 1), camera_z));
    Vec3 camera_y = vec3_normalize(vec3_cross(camera_z, camera_x));

    f32 film_dist = 1.0f;
    f32 film_w = 1.0f;
    f32 film_h = 1.0f;
    if (image->width > image->height)
    {
        film_h = film_w * (f32)image->height / (f32)image->width;
    }
    else if (image->height > image->width)
    {
        film_w = film_h * (f32)image->width / (f32)image->height;
    }

    f32 half_film_w = film_w * 0.5f;
    f32 half_film_h = film_h * 0.5f;
    Vec3 film_center = vec3_sub(camera_pos, vec3_muls(camera_z, film_dist));

    f32 pix_w = reciprocal32(image->width);
    f32 pix_h = reciprocal32(image->height);
    f32 half_pix_w = pix_w * 0.5f;
    f32 half_pix_h = pix_h * 0.5f;

    for (u32 y = ymin;
        y < one_past_ymax;
        ++y)
    {
        u32 *out = image_u32_get_pixel_pointer(image, xmin, y);
        f32 film_y = ((f32)y / (f32)image->height) * 2.0f - 1.0f;

        for (u32 x = xmin;
            x < one_past_xmax;
            ++x)
        {
            f32 film_x = ((f32)x / (f32)image->width) * 2.0f - 1.0f;

            Vec4 color = vec4(0, 0, 0, 1);
            f32 contrib = reciprocal32(rays_per_pixel);
            for (u32 ray_index = 0;
                ray_index < rays_per_pixel;
                ++ray_index)
            {
                f32 off_x = film_x + random_bilateral(&series) * half_pix_w;
                f32 off_y = film_y + random_bilateral(&series) * half_pix_h;
                Vec3 film_pos = vec3_add(film_center,
                    vec3_add(vec3_muls(camera_x, off_x * half_film_w),
                        vec3_muls(camera_y, off_y * half_film_h)));
                Vec3 ray_origin = camera_pos;
                Vec3 ray_dir    = vec3_normalize(vec3_sub(film_pos, camera_pos));

                Vec4 ray_cast_color = vec4(0, 0, 0, 1);

                u64 bounces_computed = 0;

                Ray ray;
                ray.origin = ray_origin;
                ray.dir    = ray_dir;

                f32 min_hit_distance = 0.0001f;
                f32 tolerance = 0.0001f;

                Vec4 attenuation = vec4(1, 1, 1, 1);

                for (u32 bounce_count = 0;
                    bounce_count < 8;
                    ++bounce_count)
                {
                    ++bounces_computed;

                    f32 hit_distance = F32_MAX;

                    u32 hit_mat_index = 0;
                    Vec3 next_normal ={ 0 };

                    for (u32 plane_index = 0;
                        plane_index < world->plane_count;
                        ++plane_index)
                    {
                        Plane plane = world->planes[plane_index];

                        f32 denominator = vec3_dot(plane.normal, ray.dir);
                        if ((denominator < -tolerance) >(denominator > tolerance))
                        {
                            f32 t = (-plane.dist - vec3_dot(plane.normal, ray.origin)) / denominator;
                            if ((t > min_hit_distance) && (t < hit_distance))
                            {
                                hit_distance = t;
                                hit_mat_index = plane.mat_index;

                                next_normal = plane.normal;
                            }
                        }
                    }

                    for (u32 sphere_index = 0;
                        sphere_index < world->sphere_count;
                        ++sphere_index)
                    {
                        Sphere sphere = world->spheres[sphere_index];

                        Vec3 sphere_relative_ray_origin = vec3_sub(ray.origin, sphere.pos);
                        f32 a = vec3_dot(ray.dir, ray.dir);
                        f32 b = 2.0f * vec3_dot(sphere_relative_ray_origin, ray.dir);
                        f32 c = vec3_dot(sphere_relative_ray_origin, sphere_relative_ray_origin) - square(sphere.radius);

                        f32 denominator = 2.0f * a;
                        f32 root_term = sqrt32(square(b) - 4.0f * a * c);

                        if (root_term > tolerance)
                        {
                            f32 tp = (-b + root_term) / denominator;
                            f32 tn = (-b - root_term) / denominator;

                            f32 t = tp;
                            if ((tn > min_hit_distance) && (tn < tp))
                            {
                                t = tn;
                            }

                            if ((t > min_hit_distance) && (t < hit_distance))
                            {
                                hit_distance = t;
                                hit_mat_index = sphere.mat_index;

                                next_normal = vec3_normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray.dir, hit_distance)));
                            }
                        }
                    }

                    if (hit_mat_index)
                    {
                        Material mat = world->materials[hit_mat_index];

                        ray_cast_color  = vec4_add(ray_cast_color, vec4_mul(attenuation, mat.emit_color));
                        f32 cos_atten = vec3_dot(vec3_neg(ray.dir), next_normal);
                        if (cos_atten < 0) cos_atten = 0;
                        attenuation = vec4_mul(attenuation, vec4_muls(mat.reflect_color, cos_atten));

                        ray.origin = ray_point_at(ray, hit_distance);

                        Vec3 pure_bounce = vec3_reflect(ray.dir, next_normal);
                        Vec3 random_bounce = vec3_normalize(vec3_add(next_normal,
                            vec3(random_bilateral(&series), random_bilateral(&series), random_bilateral(&series))));
                        ray.dir = vec3_normalize(vec3_lerp(random_bounce, pure_bounce, mat.scatter));
                    }
                    else
                    {
                        Material mat = world->materials[hit_mat_index];
                        ray_cast_color = vec4_add(ray_cast_color, vec4_mul(attenuation, mat.emit_color));
                        break;
                    }
                }

                color = vec4_add(color, vec4_muls(ray_cast_color, contrib));
                atomic_add64(&queue->bounces_computed, bounces_computed);
            }

            color = vec4_muls(vec4(linear_to_srgb(color.x), linear_to_srgb(color.y), linear_to_srgb(color.z), 1), 255.0f);
            u32 out_value = rgba_pack_4x8(color);
            *out++ = out_value;
        }
    }

    atomic_add64(&queue->tile_retired_count, 1);

    return true;
}

static THREAD_PROC_SIGNATURE(render_thread_proc)
{
    RenderWorkQueue *queue = param;

    while (render_tile(queue)) {}
    
    sys_exit_thread();
    return 0;
}

int
main(int argc, char **argv)
{
    printf("Ray started!\n");

    Material materials[16] ={ 0 };
    materials[0].emit_color    = vec4(0.3f, 0.4f, 0.5f, 1);
    materials[1].reflect_color = vec4(0.5f, 0.5f, 0.5f, 1);
    materials[2].reflect_color = vec4(0.7f, 0.5f, 0.3f, 1);
    materials[3].reflect_color = vec4(10.0f, 0.0f, 0.0f, 1);
    materials[4].reflect_color = vec4(0.2f, 0.8f, 0.2f, 1);
    materials[4].scatter = 0.7f;
    materials[5].reflect_color = vec4(0.4f, 0.8f, 0.9f, 1);
    materials[5].scatter = 0.85f;
    for(u32 material_index = 6;
        material_index < array_size(materials);
        ++material_index)
    {
        Material *material = materials + material_index;
        RandomSeries series = (RandomSeries) { rand() };
        
        material->scatter = random_unitlateral(&series) * 0.5f + 0.5f;
        #define random_color random_unitlateral(&series) * 0.6f + 0.4f
        material->reflect_color = vec4(random_color, random_color, random_color, 1.0f);
    }
    materials[15].reflect_color = vec4(0.95f, 0.95f, 0.95f, 1);
    materials[15].scatter = 0.99f;

    Plane planes[1] ={ 0 };
    planes[0].mat_index = 1;
    planes[0].normal = vec3(0, 0, 1);
    planes[0].dist = 0;

    Sphere spheres[55] ={ 0 };
    spheres[0].pos = vec3(-3, 2, 0);
    spheres[0].radius = 2.0f;
    spheres[0].mat_index = 15;

    for(u32 sphere_index = 1;
        sphere_index < array_size(spheres);
        ++sphere_index)
    {
        Sphere *sphere = spheres + sphere_index;
        RandomSeries series = (RandomSeries) { rand() };

        sphere->radius = random_unitlateral(&series) * 0.5f + 0.5f;
        sphere->mat_index = rand() % 14 + 1;
        Vec3 pos = vec3(random_bilateral(&series), random_bilateral(&series), random_unitlateral(&series));
        pos = vec3_muls(pos, 7.0f);
        sphere->pos = pos;
    }
    // spheres[1].pos = vec3(3, -2, 0);
    // spheres[1].radius = 1.0f;
    // spheres[1].mat_index = 3;

    // spheres[2].pos = vec3(-2, -1, 2);
    // spheres[2].radius = 1.0f;
    // spheres[2].mat_index = 4;

    // spheres[3].pos = vec3(1, -1, 3);
    // spheres[3].radius = 1.0f;
    // spheres[3].mat_index = 5;

    // spheres[4].pos = vec3(0, 0, 0);
    // spheres[4].radius = 1.0f;
    // spheres[4].mat_index = 2;


    World world ={ 0 };
    world.material_count = array_size(materials);
    world.materials = materials;
    world.plane_count = array_size(planes);
    world.planes = planes;
    world.sphere_count = array_size(spheres);
    world.spheres = spheres;

    // u32 size = 1024;
    ImageU32 image = image_u32_make(1280, 720);

    clock_t start_clock = clock();

    u32 core_count = sys_get_processor_count();
    u32 tile_width  = image.width / core_count;
    u32 tile_height = tile_width;
    tile_width = tile_height = 64;

    u32 tile_count_x = (image.width + tile_width - 1) / tile_width;
    u32 tile_count_y = (image.height + tile_height - 1) / tile_height;
    u32 total_tile_count = tile_count_x * tile_count_y;

    printf("Core count: %d, tile size: %u %ux%u (%lluk/tile)\n", core_count, total_tile_count, tile_width, tile_height,
        (tile_width * tile_height * sizeof(u32)) / 1024);

    RenderWorkQueue queue ={ 0 };
    queue.work_orders = malloc(sizeof(RenderWorkOrder) * total_tile_count);

    for (u32 tile_y = 0;
        tile_y < tile_count_y;
        ++tile_y)
    {
        u32 ymin = tile_y * tile_height;
        u32 one_past_ymax = ymin + tile_height;
        if (one_past_ymax > image.height) one_past_ymax = image.height;

        for (u32 tile_x = 0;
            tile_x < tile_count_x;
            ++tile_x)
        {
            u32 xmin = tile_x * tile_width;
            u32 one_past_xmax = xmin + tile_width;
            if (one_past_xmax > image.width) one_past_xmax = image.width;

            RenderWorkOrder *order = queue.work_orders + queue.work_order_count++;
            assert(queue.work_order_count <= total_tile_count);

            order->world = &world;
            order->image = &image;
            order->xmin = xmin;
            order->ymin = ymin;
            order->one_past_xmax = one_past_xmax;
            order->one_past_ymax = one_past_ymax;
            order->random_series.state = tile_x * 12098 + tile_y * 23771;
        }
    }

    assert(queue.work_order_count == total_tile_count);

    // @NOTE(hl): Memory fence
    atomic_add64(&queue.next_work_order_index, 0);

    for (u32 core_index = 1;
        core_index < core_count;
        ++core_index)
    {
        sys_create_thread(render_thread_proc, &queue);
    }

    while (queue.tile_retired_count < total_tile_count)
    {
        if (render_tile(&queue))
        {
            printf("\rRaycasting %d%%", 100 * (u32)queue.tile_retired_count / total_tile_count);
            fflush(stdout);
        }
    }

    clock_t end_clock = clock();
    clock_t time_elapsed = end_clock - start_clock;

    printf("\n");
    printf("Raycasting time: %lldms\n", (i64)time_elapsed);
    printf("Total bounces: %llu\n", queue.bounces_computed);
    printf("Perfomance: %fms/bounce", (f64)time_elapsed / (f64)queue.bounces_computed);

    char *filename = "out.png";
    image_u32_save(&image, filename);

    printf("\nRay finished!\n");
    return 0;
}