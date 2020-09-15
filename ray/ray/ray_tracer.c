#include "ray_tracer.h"

Ray 
camera_ray_at(Camera *camera, f32 film_x, f32 film_y, RandomSeries *series)
{
    f32 off_x = film_x + random_bilateral(series) * camera->half_pix_w;
    f32 off_y = film_y + random_bilateral(series) * camera->half_pix_h;
    Vec3 film_pos = vec3_add(camera->film_center,
                             vec3_add(vec3_muls(camera->camera_x, off_x * camera->half_film_w),
                                      vec3_muls(camera->camera_y, off_y * camera->half_film_h)));
    Vec3 ray_origin = camera->camera_pos;
    Vec3 ray_dir = normalize(vec3_sub(film_pos, camera->camera_pos));
    
    f32 time = random_range(series, camera->time0, camera->time1);
    Ray result = make_ray(ray_origin, ray_dir, time);

    return result;
}

Camera
make_camera(Vec3 pos, ImageU32 *image)
{
    Camera camera;
    camera.time0 = 0;
    camera.time1 = 1;
    // Camera is an orthographic projection with film having size of dest image
    camera.camera_pos = pos;
    // @NOTE(hl): Create coordinate system for camera.
    // These are unit vectors of 3 axes of our coordinate system
    camera.camera_z = normalize(camera.camera_pos);
    camera.camera_x = normalize(cross(vec3(0, 0, 1), camera.camera_z));
    camera.camera_y = normalize(cross(camera.camera_z, camera.camera_x));
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


void 
texture_init_solid(Texture *texture, Vec3 c)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_Solid,
    texture->solid.color = c;
}

void 
texture_init_scale(Texture *texture, Texture *texture1, Texture *texture2)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_Scale;
    texture->scale.texture1 = texture1;
    texture->scale.texture2 = texture2;
}

void 
texture_init_mix(Texture *texture, Texture *texture1, Texture *texture2, f32 mix)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_Mix;
    texture->scale.texture1 = texture1;
    texture->scale.texture2 = texture2;
    texture->scale.mix_value = mix;
}

void 
texture_init_bilerp(Texture *texture, Vec3 c00, Vec3 c01, Vec3 c10, Vec3 c11)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_BilinearInterpolation;
    texture->bilinear_interpolation.color00 = c00;   
    texture->bilinear_interpolation.color01 = c01;   
    texture->bilinear_interpolation.color10 = c10;   
    texture->bilinear_interpolation.color11 = c11;   
}

void 
texture_init_uv(Texture *texture)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_UV;
}

void 
texture_init_checkered(Texture *texture, Texture *texture1, Texture *texture2)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_Checkered;
    texture->checkered.texture1 = texture1;
    texture->checkered.texture2 = texture2;
}

void 
texture_init_image(Texture *texture, char *filename)
{
    memset(texture, 0, sizeof(*texture));
    
    texture->type = Texture_Image;
    texture->meta.image.filename = filename;
    image_u32_load(&texture->image.image, filename);
}

Vec3 
sample_texture(Texture *texture, Vec2 uv, Vec3 hit_point)
{
    if (!texture) 
    {
        return vec3(0, 0, 0);
    }
    
	Vec3 result = {0};
	
	switch(texture->type)
	{
		case Texture_Solid:
		{
			result = texture->solid.color;
		} break;
		case Texture_Scale:
		{
			result = vec3_mul(sample_texture(texture->scale.texture1, uv, hit_point),
							  sample_texture(texture->scale.texture2, uv, hit_point));
		} break;
		case Texture_Mix:
		{
			result = vec3_mix(sample_texture(texture->mix.texture1, uv, hit_point),
							  sample_texture(texture->mix.texture2, uv, hit_point),
							  texture->mix.mix_value);
		} break;
		case Texture_BilinearInterpolation:
		{
			result = vec3_add4(vec3_muls(texture->bilinear_interpolation.color00, (1 - uv.u) * (1 - uv.v)),
							   vec3_muls(texture->bilinear_interpolation.color01, (1 - uv.u) * (    uv.v)),
							   vec3_muls(texture->bilinear_interpolation.color10, (    uv.u) * (1 - uv.v)),
							   vec3_muls(texture->bilinear_interpolation.color11, (    uv.u) * (    uv.v)));
		} break;
		case Texture_UV:
		{
			result.r = uv.u;
			result.g = uv.v;
			result.b = 0;
		} break;
		case Texture_Checkered:
		{
			if ((mod32(abs32(hit_point.x + 10000.0f), 2.0f) > 1.0f) - 
                (mod32(abs32(hit_point.y + 10000.0f), 2.0f) > 1.0f))
			{
				result = sample_texture(texture->checkered.texture1, uv, hit_point);
			}
			else 
			{
				result = sample_texture(texture->checkered.texture2, uv, hit_point);
			}
		} break;
		case Texture_Image:
		{
			ImageU32 *image = &texture->image.image;
			
			uv.u = 1.0f - clamp(uv.u, 0, 1);
			uv.v = 1.0f - clamp(uv.v, 0, 1);
			
			u32 x = round32(uv.u * image->width);
			u32 y = round32(uv.v * image->height);
			
			if (x >= image->width) 
			{
				x = image->width - 1;
			}
			if (y >= image->height)
			{
				y = image->height - 1;
			}
			
			f32 r255 = reciprocal32(255.0f);
			u8 *pixels = (u8 *)image_u32_get_pixel_pointer(image, x, y);
			
			result.x = pixels[0] * r255;
			result.y = pixels[1] * r255;
			result.z = pixels[2] * r255;
		} break;
	}
	
	return result;
}


void 
object_init_sphere(Object *object, Transform transform, u32 mat_index, Sphere sphere)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Sphere;
    object->transform = transform;
    object->mat_index = mat_index;
    object->sphere = sphere;
}

void 
object_init_plane(Object *object, Transform transform, u32 mat_index, Plane plane)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Plane;
    object->transform = transform;
    object->mat_index = mat_index;
    object->plane = plane;
}

void 
object_init_disk(Object *object, Transform transform, u32 mat_index, Disk disk)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Disk;
    object->transform = transform;
    object->mat_index = mat_index;
    object->disk = disk;
}

void 
object_init_triangle(Object *object, Transform transform, u32 mat_index, Triangle triangle)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Triangle;
    object->transform = transform;
    object->mat_index = mat_index;
    object->triangle = triangle;
}

void 
object_init_cylinder(Object *object, Transform transform, u32 mat_index, Cylinder cylinder)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Cylinder;
    object->transform = transform;
    object->mat_index = mat_index;
    object->cylinder = cylinder;
}

void 
object_init_cone(Object *object, Transform transform, u32 mat_index, Cone cone)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Cone;
    object->transform = transform;
    object->mat_index = mat_index;
    object->cone = cone;
}

void 
object_init_hyperboloid(Object *object, Transform transform, u32 mat_index, Hyperboloid hyperboloid)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Hyperboloid;
    object->transform = transform;
    object->mat_index = mat_index;
    object->hyperboloid = hyperboloid;
}

void 
object_init_paraboloid(Object *object, Transform transform, u32 mat_index, Paraboloid paraboloid)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_Paraboloid;
    object->transform = transform;
    object->mat_index = mat_index;
    object->paraboloid = paraboloid;
}

void 
object_init_triangle_mesh(Object *object, Transform transform, u32 mat_index, TriangleMesh mesh)
{
    memset(object, 0, sizeof(*object));

    object->type = Object_TriangleMesh;
    object->transform = transform;
    object->mat_index = mat_index;
    object->triangle_mesh = mesh;
}

