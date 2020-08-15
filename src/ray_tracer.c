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
