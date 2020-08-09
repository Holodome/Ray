#include "Ray.h"

ImageU32 
image_u32_make(u32 width, u32 height)
{
    ImageU32 result;
    
    result.width = width;
    result.height = height;
    result.pixels = malloc(sizeof(u32) * width * height);
    
    return result;
}

void     
image_u32_save(ImageU32 *image, char *filename)
{
    stbi_write_png("out.png", image->width, image->height, 4, image->pixels, sizeof(u32) * image->width);
}

static Vec4
ray_cast(World *world, Vec3 ray_origin, Vec3 ray_dir)
{
    Vec4 result = {0};
    
    return result;
}

int 
main(int argc, char **argv)
{
    printf("Ray started!\n");
    
    Material materials[2] = {0};
    materials[0].color = vec4(0, 0, 0, 1);
    materials[1].color = vec4(1, 0, 0, 1);
    
    Plane plane = {0};
    plane.mat_index = 1;
    
    World world = {0};
    world.material_count = array_size(materials);
    world.materials = materials;
    world.plane_count = 1;
    world.planes = &plane;
    world.sphere_count = 0;
    world.spheres = 0;
    
    Vec3 camera_pos = vec3(0, 10, 1);
    // @NOTE(hl): Create coordinate system from camera.
    // These are unit vectors of 3 axes of our coordinate system
    Vec3 camera_z = vec3_normalize_slow(camera_pos);
    Vec3 camera_x = vec3_normalize_slow(vec3_cross_product(camera_z, vec3(0, 0, 1)));
    Vec3 camera_y = vec3_normalize_slow(vec3_cross_product(camera_z, camera_x));
    
    f32 film_dist    = 1.0f;
    f32 film_w = 1.0f;
    f32 film_h = 1.0f;
    f32 half_film_w = film_w * 0.5f;
    f32 half_film_h = film_h * 0.5f;
    Vec3 film_center = vec3_sub(camera_pos, vec3_mul(camera_z, vec3s(film_dist)));
    
    ImageU32 image = image_u32_make(1280, 720);
    
    u32 *out = image.pixels;
    for(u32 x = 0;
        x < image.width;
        ++x)
    {
        f32 film_x = ((f32)x / (f32)image.width) * 2.0f - 1.0f;
        for(u32 y = 0;
            y < image.height;
            ++y)
        {
            f32 film_y = ((f32)y / (f32)image.height) * 2.0f - 1.0f;
            
            Vec3 film_pos = vec3_add(film_center, 
                                     vec3_add(vec3_mul(camera_x, vec3s(film_x * half_film_w)), 
                                              vec3_mul(camera_y, vec3s(film_y * half_film_h))));
            Vec3 ray_origin = camera_pos;
            Vec3 ray_dir    = vec3_normalize_slow(vec3_sub(film_pos, camera_pos));
            
            Vec4 color = ray_cast(&world, ray_origin, ray_dir);
            
            u32  out_value = vec4_normalized_to_u32(color);
            *out++ = out_value;
        }
    } 
    
    image_u32_save(&image, "out.png");
    
    printf("Ray finished!\n");
    return 0;
}