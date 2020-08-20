#include "ray_tracer.h"

inline Ray 
ray(Vec3 origin, Vec3 dir, f32 time)
{
    Ray result = {
        .origin = origin,
        .dir = dir,
        .time = time
    };
    return result;
}

Vec3 
moving_sphere_center(MovingSphere *sphere, f32 time)
{
    f32 dt0 = time - sphere->time0;
    f32 dt1 = sphere->time1 - sphere->time0;
    Vec3 dcent = vec3_sub(sphere->center1, sphere->center0);
    Vec3 result = vec3_add(sphere->center0, vec3_muls(dcent, dt0 / dt1));
    return result;
}

Ray 
camera_ray_at(Camera *camera, f32 film_x, f32 film_y, RandomSeries *series)
{
    Ray ray = {0};

    f32 off_x = film_x + random_bilateral(series) * camera->half_pix_w;
    f32 off_y = film_y + random_bilateral(series) * camera->half_pix_h;
    Vec3 film_pos = vec3_add(camera->film_center,
                             vec3_add(vec3_muls(camera->camera_x, off_x * camera->half_film_w),
                                      vec3_muls(camera->camera_y, off_y * camera->half_film_h)));
    Vec3 ray_origin = camera->camera_pos;
    Vec3 ray_dir = vec3_normalize(vec3_sub(film_pos, camera->camera_pos));

    ray.origin = ray_origin;
    ray.dir = ray_dir;
    ray.time = random_range(series, camera->time0, camera->time1);

    return ray;
}

Camera
camera(Vec3 pos, ImageU32 *image)
{
    Camera camera;
    camera.time0 = 0;
    camera.time1 = 1;
    // Camera is an orthographic projection with film having size of dest image
    camera.camera_pos = pos;
    // @NOTE(hl): Create coordinate system for camera.
    // These are unit vectors of 3 axes of our coordinate system
    camera.camera_z = vec3_normalize(camera.camera_pos);
    camera.camera_x = vec3_normalize(cross(vec3(0, 0, 1), camera.camera_z));
    camera.camera_y = vec3_normalize(cross(camera.camera_z, camera.camera_x));
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
    return camera;
}