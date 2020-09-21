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


static TextureID
textures_get_next_id(TextureArray *textures)
{
    TextureID result = { INVALID_ID };
    
    if (textures->count + 1 < textures->max_count)
    {
        result.id = textures->count++;
    }
    else 
    {
        fprintf(stderr, "[ERROR] Texture array %p size exceeded\n", textures);
    }
    
    return result;
}

TextureID 
textures_append_solid(TextureArray *textures, Vec3 c)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_Solid,
        texture->solid.color = c;
    }
    
    return id;
}

TextureID 
textures_append_scale(TextureArray *textures, TextureID texture1, TextureID texture2)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
        
        texture->type = Texture_Scale;
        texture->scale.texture1 = texture1;
        texture->scale.texture2 = texture2;
    }
    
    return id;
}

TextureID 
textures_append_mix(TextureArray *textures, TextureID texture1, TextureID texture2, f32 mix)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_Mix;
        texture->mix.texture1 = texture1;
        texture->mix.texture2 = texture2;
        texture->mix.mix_value = mix;
    }
    
    return id;
}

TextureID 
textures_append_bilerp(TextureArray *textures, Vec3 c00, Vec3 c01, Vec3 c10, Vec3 c11)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_BilinearInterpolation;
        texture->bilinear_interpolation.color00 = c00;   
        texture->bilinear_interpolation.color01 = c01;   
        texture->bilinear_interpolation.color10 = c10;   
        texture->bilinear_interpolation.color11 = c11;   
    }
    
    return id;
}

TextureID 
textures_append_uv(TextureArray *textures)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_UV;
    }
    
    return id;
}

TextureID 
textures_append_checkered(TextureArray *textures, TextureID texture1, TextureID texture2)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_Checkered;
        texture->checkered.texture1 = texture1;
        texture->checkered.texture2 = texture2;
    }
    
    return id;
}

TextureID 
textures_append_image(TextureArray *textures, char *filename)
{
    TextureID id = textures_get_next_id(textures);
    if (is_valid_id(id))
    {
        Texture *texture = textures->textures + id.id;
        memset(texture, 0, sizeof(*texture));
    
        texture->type = Texture_Image;
        texture->meta.image.filename = filename;
        image_u32_load(&texture->image.image, filename);
    }
    
    return id;
}


Vec3 
sample_texture(TextureArray *textures, TextureID id, Vec2 uv, Vec3 hit_point)
{
    if (!is_valid_id(id))
    {
        return vec3(0, 0, 0);
    }
    
	Vec3 result = {0};
	
    Texture *texture = textures->textures + id.id;
	switch(texture->type)
	{
		case Texture_Solid:
		{
			result = texture->solid.color;
		} break;
		case Texture_Scale:
		{
			result = vec3_mul(sample_texture(textures, texture->scale.texture1, uv, hit_point),
							  sample_texture(textures, texture->scale.texture2, uv, hit_point));
		} break;
		case Texture_Mix:
		{
			result = vec3_mix(sample_texture(textures, texture->mix.texture1, uv, hit_point),
							  sample_texture(textures, texture->mix.texture2, uv, hit_point),
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
				result = sample_texture(textures, texture->checkered.texture1, uv, hit_point);
			}
			else 
			{
				result = sample_texture(textures, texture->checkered.texture2, uv, hit_point);
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


MaterialID 
materials_append(MaterialArray *materials, Material material)
{
    MaterialID result = { INVALID_ID };
    
    if (materials->count + 1 < materials->max_count)
    {
        result.id = materials->count++;
        
        *(materials->materials + result.id) = material;
    }
    else 
    {
        fprintf(stderr, "[ERROR] Material array %p size exceeded\n", materials);
    }
    
    return result;
}




Object 
make_object_sphere(Transform transform, MaterialID mat_id, Sphere sphere)
{
    Object obj = {0};
    
    obj.type = Object_Sphere;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.sphere = sphere;
    
    return obj;
}

Object 
make_object_plane(Transform transform, MaterialID mat_id, Plane plane)
{
    Object obj = {0};
    
    obj.type = Object_Plane;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.plane = plane;
    
    return obj;   
}

Object 
make_object_disk(Transform transform, MaterialID mat_id, Disk disk)
{
    Object obj = {0};
    
    obj.type = Object_Disk;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.disk = disk;
    
    return obj;  
}

Object 
make_object_triangle(Transform transform, MaterialID mat_id, Triangle triangle)
{
    Object obj = {0};
    
    obj.type = Object_Triangle;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.triangle = triangle;
    
    return obj;    
}

Object 
make_object_cylinder(Transform transform, MaterialID mat_id, Cylinder cylinder)
{
    Object obj = {0};
    
    obj.type = Object_Cylinder;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.cylinder = cylinder;
    
    return obj;   
}

Object 
make_object_cone(Transform transform, MaterialID mat_id, Cone cone)
{
    Object obj = {0};
    
    obj.type = Object_Cone;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.cone = cone;
    
    return obj;  
}

Object 
make_object_hyperboloid(Transform transform, MaterialID mat_id, Hyperboloid hyperboloid)
{
    Object obj = {0};
    
    obj.type = Object_Hyperboloid;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.hyperboloid = hyperboloid;
    
    return obj;    
}

Object 
make_object_paraboloid(Transform transform, MaterialID mat_id, Paraboloid paraboloid)
{
    Object obj = {0};
    
    obj.type = Object_Paraboloid;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.paraboloid = paraboloid;
    
    return obj;    
}

Object 
make_object_triangle_mesh(Transform transform, MaterialID mat_id, TriangleMesh mesh)
{
    Object obj = {0};
    
    obj.type = Object_TriangleMesh;
    obj.transform = transform;
    obj.mat_id = mat_id;
    obj.triangle_mesh = mesh;
    
    return obj;    
}



ObjectID 
objects_append(ObjectArray *objects, Object object)
{
    ObjectID result = { INVALID_ID };
    
    if (objects->count + 1 < objects->max_count)
    {
        u32 index = result.id = objects->count++;
        *(objects->objects + index) = object;
    }
    else 
    {
        fprintf(stderr, "[ERROR] Object array %p size exceeded\n", objects);
    }
    
    return result;
}