#include "ray.h"

#include "thirdparty/stb_image_write.h"

#include "ray_math.c"
#include "sys.c"

Vec3
ray_point_at(Ray ray, f32 param)
{
    Vec3 result = vec3_add(ray.origin, vec3_muls(ray.dir, param));
    return result;
}

void
image_u32_init(ImageU32 *image, u32 width, u32 height)
{
    image->width = width;
    image->height = height;
    image->pixels = malloc(sizeof(u32) * width * height);
}

u32 *
image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y)
{
	assert((x < image->width) && (y < image->height));
	
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
        fprintf(stderr, "[ERROR] Failed to save image '%s'\n", filename);
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

static void 
cast_sample_rays(CastState *state)
{
    Scene *scene = state->scene;
    u32 rays_per_pixel = state->rays_per_pixel;
    u32 max_bounce_count = state->max_bounce_count;
    Vec3 camera_pos = state->camera_pos;
    Vec3 camera_x = state->camera_x;
    Vec3 camera_y = state->camera_y;
    Vec3 camera_z = state->camera_z;
    f32 film_w = state->film_w;
    f32 film_h = state->film_h;
    f32 half_film_w = state->half_film_w;
    f32 half_film_h = state->half_film_h;
    Vec3 film_center = state->film_center;
    f32 half_pix_w = state->half_pix_w;
    f32 half_pix_h = state->half_pix_h;
    RandomSeries series = state->series;
    f32 film_x = state->film_x;
    f32 film_y = state->film_y;
    Vec3 final_color = vec3(0, 0, 0);
    u64  bounces_computed = 0;
    
    u32 lane_width = 1;
    u32 lane_ray_count = rays_per_pixel / lane_width;
    f32 contrib = reciprocal32(lane_ray_count);
    for (u32 ray_index = 0;
		 ray_index < lane_ray_count;
		 ++ray_index)
    {
        f32 off_x = film_x + random_bilateral(&series) * half_pix_w;
        f32 off_y = film_y + random_bilateral(&series) * half_pix_h;
        Vec3 film_pos = vec3_add(film_center,
								 vec3_add(vec3_muls(camera_x, off_x * half_film_w),
										  vec3_muls(camera_y, off_y * half_film_h)));
        Vec3 ray_origin = camera_pos;
        Vec3 ray_dir    = vec3_normalize(vec3_sub(film_pos, camera_pos));
		
        Vec3 ray_cast_color = vec3(0, 0, 0);
		
        Ray ray = {
            .origin = ray_origin,
            .dir    = ray_dir  
        };
		
        Vec3 attenuation = vec3(1, 1, 1);
		
        for (u32 bounce_count = 0;
			 bounce_count < max_bounce_count;
			 ++bounce_count)
        {
            ++bounces_computed;
		
            f32 min_hit_distance = 0.0001f;
            f32 tolerance        = 0.0001f;
            	
			HitRecord hit_record = { 0 };
            hit_record.distance = F32_MAX;
            
            //
            // Iterate planes in scene
            //
            for (u32 plane_index = 0;
				 plane_index < scene->plane_count;
				 ++plane_index)
            {
                Plane plane = scene->planes[plane_index];
				
                f32 denominator = vec3_dot(plane.normal, ray.dir);
                if ((denominator < -tolerance) > (denominator > tolerance))
                {
                    f32 t = (-plane.dist - vec3_dot(plane.normal, ray.origin)) / denominator;
                    if ((t > min_hit_distance) && (t < hit_record.distance))
                    {
                        hit_record.distance = t;
                        hit_record.mat_index = plane.mat_index;
                        hit_record.normal = plane.normal;
                    }
                }
            }
			
            // 
            // Iterate spheres in scene 
            // 
            for (u32 sphere_index = 0;
				 sphere_index < scene->sphere_count;
				 ++sphere_index)
            {
                Sphere sphere = scene->spheres[sphere_index];
				
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
					
                    if ((t > min_hit_distance) && (t < hit_record.distance))
                    {
                        hit_record.distance = t;
                        hit_record.mat_index = sphere.mat_index;
                        hit_record.normal = vec3_normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray.dir, hit_record.distance)));
                    }
                }
            }
			
            if (has_hit(hit_record))
            {
                Material mat = scene->materials[hit_record.mat_index];
				
                ray.origin = ray_point_at(ray, hit_record.distance);
				
                // Decide if we want to refract or reflect
                // Check if refraction_probability != 0 (it is OK to compare floats against constants)
                if (0)
                // if ((mat.refraction_probability != 0.0f)) //&& (random_unitlateral(&series) <= mat.refraction_probability))
                {
                    Vec3 reflected = vec3_reflect(ray.dir, hit_record.normal);
                    // Glass
                    f32 ref_idx = 1.5f;
                    f32 refract_coef;
                    f32 cos_atten;
                    if (vec3_dot(ray.dir, hit_record.normal) > 0)
                    {
                        refract_coef = ref_idx;
                        cos_atten = ref_idx * vec3_dot(ray.dir, hit_record.normal);
                        hit_record.normal = vec3_neg(hit_record.normal);
                    }
                    else 
                    {
                        refract_coef = 1.0f / ref_idx;
                        cos_atten = -vec3_dot(ray.dir, hit_record.normal);
                    }
                    
                    f32 reflect_prob;
                    f32 dt = vec3_dot(ray.dir, hit_record.normal);
                    f32 discriminant = 1.0f - refract_coef * refract_coef * (1.0f - dt * dt);
                    Vec3 refracted;
                    
                    if (discriminant > 0.0f)
                    {
                        refracted = vec3_sub(vec3_muls(vec3_sub(ray.dir, vec3_muls(hit_record.normal, dt)), refract_coef), vec3_muls(hit_record.normal, sqrt32(discriminant)));
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
                    attenuation = vec3_mul(attenuation, vec3_muls(mat.reflect_color, cos_atten));

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
                                 linear_to_srgb(final_color.z)), 255.0f);
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
	
    CastState state = { 0 };
	
    RenderWorkOrder *order = queue->work_orders + work_order_index;
    
    state.scene  = order->scene;
    state.series = order->random_series;
    // @TODO(hl): Settings for these fellas
    state.rays_per_pixel   = 256;
    state.max_bounce_count = 8;
    
    ImageU32 *image = order->image;
    state.half_pix_w = reciprocal32(image->width)  * 0.5f;
    state.half_pix_h = reciprocal32(image->height) * 0.5f;
	
    // @TODO(hl): Make camera pos and view easilly changable
    // Camera is an orthographic projection with film having size of dest image
    state.camera_pos = vec3(0, -10, 1);
    // @NOTE(hl): Create coordinate system for camera.
    // These are unit vectors of 3 axes of our coordinate system
    state.camera_z = vec3_normalize(state.camera_pos);
    state.camera_x = vec3_normalize(vec3_cross(vec3(0, 0, 1), state.camera_z));
    state.camera_y = vec3_normalize(vec3_cross(state.camera_z, state.camera_x));
    state.film_center = vec3_sub(state.camera_pos, state.camera_z);
    
	// Apply image aspect ratio
    state.film_w = 1.0f;
    state.film_h = 1.0f;
    if (image->width > image->height)
    {
        state.film_h = state.film_w * (f32)image->height / (f32)image->width;
    }
    else if (image->height > image->width)
    {
        state.film_w = state.film_h * (f32)image->width / (f32)image->height;
    }
    state.half_film_w = state.film_w * 0.5f;
    state.half_film_h = state.film_h * 0.5f;
	
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
	
    while (render_tile(queue)) {}
    
    sys_exit_thread();
    return 0;
}

// Returns scene with same objects in it
// Can be used for testing and comparing results
void
init_sample_scene(Scene *scene)
{
	// @NOTE(hl): Don't malloc arrays because scene settings is always the same
	static Material materials[7] = { 0 };
    materials[0].emit_color    = vec3(0.3f, 0.4f, 0.5f);
    materials[1].reflect_color = vec3(0.5f, 0.5f, 0.5f);
    materials[2].reflect_color = vec3(0.7f, 0.5f, 0.3f);
    materials[2].refraction_probability = 1.0f;
    materials[3].reflect_color = vec3(10.0f, 0.0f, 0.0f);
    materials[4].reflect_color = vec3(0.2f, 0.8f, 0.2f);
    materials[4].scatter = 0.7f;
    materials[5].reflect_color = vec3(0.4f, 0.8f, 0.9f);
    materials[5].scatter = 0.85f;
    materials[6].reflect_color = vec3(0.95f, 0.95f, 0.95f);
    materials[6].scatter = 0.99f;
	
    static Plane planes[1] = { 0 };
    planes[0].mat_index = 1;
    planes[0].normal = vec3(0, 0, 1);
    planes[0].dist = 0;
	
    static Sphere spheres[5] = { 0 };
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
	
    spheres[4].pos = vec3(-1, -3, 1);
    spheres[4].radius = 1.0f;
    spheres[4].mat_index = 2;
	
	scene->material_count = array_size(materials);
	scene->materials = materials;
	scene->sphere_count = array_size(spheres);
	scene->spheres = spheres;
	scene->plane_count = array_size(planes);
	scene->planes = planes;
}

void 
init_random_scene(Scene *scene)
{
	u32 material_count = 20;
	u32 sphere_count   = 30;
	u32 plane_count    = 1;
	
	f32 pos_range = 7.0f;
	
	Material *materials = malloc(sizeof(Material) * material_count);
	Sphere   *spheres   = malloc(sizeof(Sphere) * sphere_count);
	Plane    *planes    = malloc(sizeof(Plane) * plane_count);
	
	RandomSeries series = { time(0) };
	
    materials[0].emit_color = vec3(0.3f, 0.4f, 0.5f);
	for(u32 material_index = 1;
		material_index < material_count;
		++material_index)
	{
		Material *material = materials + material_index;
		material->scatter = random_range(&series, 0.5f, 1.0f);
		material->reflect_color = vec3(random_range(&series, 0.4f, 1.0f),
									   random_range(&series, 0.4f, 1.0f),
									   random_range(&series, 0.4f, 1.0f));
	}
	
	for(u32 sphere_index = 0;
		sphere_index < sphere_count;
		++sphere_index)
	{
		Sphere *sphere = spheres + sphere_index;
		
		sphere->radius = random_unitlateral(&series) * 0.5f + 0.5f;
		sphere->mat_index = xorshift32(&series) % material_count;
		Vec3 unit_pos = vec3(random_bilateral(&series), random_bilateral(&series), random_unitlateral(&series));
		sphere->pos = vec3_muls(unit_pos, pos_range);
	}
	
    planes[0].mat_index = 1;
    planes[0].normal = vec3(0, 0, 1);
    planes[0].dist = 0;
	
	scene->material_count = material_count;
	scene->materials = materials;
	scene->sphere_count = sphere_count;
	scene->spheres = spheres;
	scene->plane_count = plane_count;
	scene->planes = planes;
}

int
main(int argc, char **argv)
{
    printf("Ray started!\n");
	
    // Initialize scene
    Scene scene = { 0 };
	init_sample_scene(&scene);
	
    // Raycasting output is simply an image  
    ImageU32 image = { 0 };
	image_u32_init(&image, 1280, 720);
	
    // Record time spend on raycasting
    clock_t start_clock = clock();
	
    // Get core count to initialize threads
    u32 core_count = sys_get_processor_count();
    // Decide how image is divided in tiles that will be given to threads
    u32 tile_width = 64, tile_height = 64;
	// Calculate tile count
    u32 tile_count_x = (image.width  + tile_width  - 1) / tile_width;
    u32 tile_count_y = (image.height + tile_height - 1) / tile_height;
    u32 total_tile_count = tile_count_x * tile_count_y;
	
    printf("Core count: %d, tile size: %u %ux%u (%lluk/tile)\n", core_count, total_tile_count, tile_width, tile_height,
		   (tile_width * tile_height * sizeof(u32)) / 1024);
	
    // Initialize multithreaded work orders for rendering
    // Because all tiles respond for different pixel groups with no intersections, we can have 
    // lockless write access to image. And since we also only read from scene structure, 
    // we have no locks at all. 
    RenderWorkQueue queue = { 0 };
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
            order->random_series = (RandomSeries){ tile_x * 12098 + tile_y * 23771 } ;
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
            printf("\rRaycasting %d%%", 100 * (u32)queue.tile_retired_count / total_tile_count);
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