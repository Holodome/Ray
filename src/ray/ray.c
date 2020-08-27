#include "ray/ray.h"

#include "lib/ray_math.c"
#include "lib/sys.c"
#include "lib/memory_pool.c"

#include "image.c"
#include "ray/ray_tracer.c"

#include "ray/utah_teapot.c"

// #include "ray/scene_file.c"

const f32 min_hit_distance = 0.001f;
const f32 tolerance        = 0.001f;

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
static inline bool
ray_intersect_triangle(Ray ray, Vec3 v0, Vec3 v1, Vec3 v2,
                       f32 *dt, f32 *du, f32 *dv)
{
    bool result = false;
    
    Vec3 edge1 = vec3_sub(v1, v0);
    Vec3 edge2 = vec3_sub(v2, v0);
    Vec3 h = cross(ray.direction, edge2);
    f32  a = dot(edge1, h);
    
#if 1
    if ((a < -tolerance) || (a > tolerance))
#else 
    // @NOTE(hl): This is culling. Discards back faces
    if (a > tolerance)
#endif 
    {
        f32  f = 1.0f / a;
        Vec3 s = vec3_sub(ray.origin, v0);
        f32  u = f * dot(s, h);
        Vec3 q = cross(s, edge1);
        f32 v = f * dot(ray.direction, q);
        f32 t = f * dot(edge2, q);
        // @NOTE(hl): Although it looks messy, it speeds up perfomance significantly by reducing ifs
        if (((0 < u) && (u < 1)) && ((v > 0) && (u + v < 1)))
        {
            *dt = t;
            *du = u;
            *dv = v;
            result = true;
        }
    }
    
    return result;
}

static bool 
ray_intersect_extents(Ray ray, Extents *extents, 
                      f32 precomputed_numerator  [static BVH_NUM_PLANE_SET_NORMALS], 
                      f32 precomputed_denominator[static BVH_NUM_PLANE_SET_NORMALS],
                      f32 *tnear, f32 *tfar, u32 *plane_index)
{
    bool result = true;
    
    for (u32 i = 0;
         i < BVH_NUM_PLANE_SET_NORMALS;
         ++i)
    {
        f32 tmin = (extents->dmin[i] - precomputed_numerator[i]) / precomputed_denominator[i];
        f32 tmax = (extents->dmax[i] - precomputed_numerator[i]) / precomputed_denominator[i];
        if (precomputed_denominator[i] < 0)
        {
            swap(tmin, tmax);
        }
		
        if (tmin > *tnear)
        {
            *tnear = tmin;
            *plane_index = i;
        }
        if (tmax < *tfar)
        {
            *tfar = tmax;
        }
        
        if (*tnear > *tfar)
        {
            result = false;
            break;       
        }
    }
    return result;
}

static Vec3  
ray_cast(CastState *state, Ray ray, HitRecord *hit_record)
{    
    Scene *scene = state->scene;
    
    Vec3 ray_cast_color = {0};
    
    f32 precomputed_numerator[BVH_NUM_PLANE_SET_NORMALS];
    f32 precomputed_denumerator[BVH_NUM_PLANE_SET_NORMALS];
    for (u32 i = 0;
		 i < BVH_NUM_PLANE_SET_NORMALS;
		 ++i)
    {
        precomputed_numerator[i]   = dot(plane_set_normals[i], ray.origin);
        precomputed_denumerator[i] = dot(plane_set_normals[i], ray.direction);
    }
    
    // Iterate scene objects
    for (u32 object_index = 0;
         object_index < scene->object_count;
         ++object_index)
    {
        Object object = scene->objects[object_index];
        
        switch(object.type)
        {
            case Object_Sphere:
            {
                Sphere sphere = object.sphere;
                Vec3 sphere_relative_ray_origin = vec3_sub(ray.origin, sphere.pos);
                f32 a = dot(ray.direction, ray.direction);
                f32 b = 2.0f * dot(sphere_relative_ray_origin, ray.direction);
                f32 c = dot(sphere_relative_ray_origin, sphere_relative_ray_origin) - sphere.radius * sphere.radius;
                
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
                    
                    if ((t > min_hit_distance) && (t < hit_record->distance))
                    {
                        hit_record->distance = t;
                        hit_record->mat_index = object.mat_index;
                        // hit_record_set_normal(&hit_record, ray, normalize(vec3_add(sphere_relative_ray_origin, vec3_muls(ray.dir, hit_record->distance))));
                        Vec3 normal = normalize(vec3_add(sphere_relative_ray_origin,
                                                        vec3_muls(ray.direction, hit_record->distance)));
                        // @NOTE(hl): This should be extracted from previous computations
                        hit_record_set_normal(hit_record, ray, normal);
                        hit_record->normal = normal;
                        //hit_record->ray_dir_normal_dot = dot(hit_record->normal, ray.direction);
                        hit_record->hit_point = ray_point_at(ray, t);
                        hit_record->uv = unit_sphere_get_uv(vec3_divs(vec3_sub(hit_record->hit_point, sphere.pos),
                                                                    sphere.radius));
                    }
                }
            } break;
            case Object_Plane:
            {
                Plane plane = object.plane;
                
                f32 denominator = dot(plane.normal, ray.direction);
                if ((denominator < -tolerance) || (denominator > tolerance))
                {
                    f32 t = (-plane.dist - dot(plane.normal, ray.origin)) / denominator;
                    if ((t > min_hit_distance) && (t < hit_record->distance))
                    {
                        hit_record->distance = t;
                        hit_record->mat_index = object.mat_index;
                        // hit_record->ray_dir_normal_dot = denominator;
                        hit_record_set_normal(hit_record, ray, plane.normal);
                        hit_record->hit_point = ray_point_at(ray, t);
                        // @TODO(hl): These are not normalized!!
                        hit_record->uv.u = hit_record->hit_point.x;
                        hit_record->uv.v = hit_record->hit_point.y;
                    }
                }
            } break;
            case Object_Disk:
            {
                Disk disk = object.disk;
                
                f32 denominator = dot(disk.normal, ray.direction);
                if ((denominator < -tolerance) || (denominator > tolerance))
                {
                    Vec3 p010 = vec3_sub(disk.point, ray.origin);
                    f32 t = dot(p010, disk.normal) / denominator;
                    if ((t > min_hit_distance) && (t < hit_record->distance))
                    {
                        Vec3 hit_point = ray_point_at(ray, t);
                        Vec3 dist_to_center = vec3_sub(hit_point, disk.point);
                        if (dot(dist_to_center, dist_to_center) < square(disk.radius))
                        {
                            hit_record->distance = t;
                            hit_record->mat_index = object.mat_index;
                            //hit_record->ray_dir_normal_dot = denominator;
                            hit_record_set_normal(hit_record, ray, disk.normal);
                            hit_record->hit_point = hit_point;
                            // @TODO(hl): UV
                        }
                    }
                }
            } break;
            case Object_Triangle:
            {
                Triangle triangle = object.triangle;
                
                f32 t, u, v;
                if (ray_intersect_triangle(ray, triangle.vertex0, triangle.vertex1, triangle.vertex2, &t, &u, &v))
                {
                    if ((t > min_hit_distance) && (t < hit_record->distance))
                    {
                        hit_record->distance = t;
                        hit_record->mat_index = object.mat_index;
                        hit_record_set_normal(hit_record, ray, triangle.normal);
                        hit_record->hit_point = ray_point_at(ray, t);
                        hit_record->uv = vec2(u, v);
                    }
                }
            } break;
            case Object_Cylinder:
            {
                Cylinder cylinder = object.cylinder;
            } break;
            case Object_Cone:
            {
                Cone cone = object.cone;
            } break;
            case Object_Hyperboloid:
            {
                Hyperboloid hyperboloid = object.hyperboloid;
            } break;
            case Object_Paraboloid:
            {
                Paraboloid paraboloid = object.paraboloid;
            } break;
            case Object_TriangleMesh:
            {
                TriangleMesh mesh = object.triangle_mesh;
                
                 
                f32 tnear = F32_MIN;
                f32 tfar  = F32_MAX;
                u32 plane_index;
                if (ray_intersect_extents(ray, &mesh.bvh.extents, 
                                        precomputed_numerator, precomputed_denumerator, 
                                        &tnear, &tfar, &plane_index))
                {
#if 0 
                    // This code path for testing intersection with just bounding volume
                    f32 t = tnear;
                    if ((t > min_hit_distance) && (t < hit_record->distance))
                    {
                        assert(plane_index < BVH_NUM_PLANE_SET_NORMALS);
                        Vec3 normal = plane_set_normals[plane_index];
                        
                        hit_record->distance = t;
                        hit_record->mat_index = mesh.mat_index;
                        
                        hit_record_set_normal(&hit_record, ray, normal);
                        hit_record->hit_point = ray_point_at(ray, t);
                        // hit_record->uv = vec2(u, v);
                    }
#else               
                    u32 j = 0;
                    for (u32 triangle_index = 0;
                        triangle_index < mesh.triangle_count;
                        ++triangle_index)
                    {
                        Vec3 v0 = mesh.p[mesh.tri_indices[j]];   
                        Vec3 v1 = mesh.p[mesh.tri_indices[j + 1]];   
                        Vec3 v2 = mesh.p[mesh.tri_indices[j + 2]];   
                        f32 t, u, v;
                        if (ray_intersect_triangle(ray, v0, v1, v2, &t, &u, &v))
                        {
                            if ((t > min_hit_distance) && (t < hit_record->distance))
                            {
#if 1
                                // @NOTE(hl): This does not interpolate between vertices' normals so model is kinda rough
                                Vec3 normal = triangle_normal(v0, v1, v2);
#else 
                                // @TODO(hl): This is busted
                                Vec3 n0 = mesh.n[mesh.tri_indices[triangle_index * 3]]; 
                                Vec3 n1 = mesh.n[mesh.tri_indices[triangle_index * 3 + 1]]; 
                                Vec3 n2 = mesh.n[mesh.tri_indices[triangle_index * 3 + 2]]; 
                                Vec3 normal = vec3_add3(vec3_muls(n0, 1 - u - v), vec3_muls(n1, u), vec3_muls(n2, v)); 
#endif  
                                
                                hit_record->distance = t;
                                hit_record->mat_index = object.mat_index;
                                //hit_record->ray_dir_normal_dot = dot(normal, ray.direction);
                                hit_record_set_normal(hit_record, ray, normal);
                                hit_record->hit_point = ray_point_at(ray, t);
                                hit_record->uv = vec2(u, v);
                            }
                        }
                        
                        j += 3;
                    }
#endif 
                }
            } break;
        }
    }
    
    return ray_cast_color;
}

ATTRIBUTE(flatten) ATTRIBUTE(hot)
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
	
    f32 contrib = reciprocal32(rays_per_pixel);
    for (u32 ray_index = 0;
         ray_index < rays_per_pixel;
         ++ray_index)
    {
        Ray ray = camera_ray_at(camera, film_x, film_y, &series);
        
        Vec3 ray_cast_color = vec3(0, 0, 0);
        Vec3 attenuation    = vec3(1, 1, 1);
		
        for (u32 bounce_count = 0;
			 bounce_count < max_bounce_count;
			 ++bounce_count)
        {
            ++bounces_computed;
            
            HitRecord hit_record = {0};
            hit_record.distance = F32_MAX;
            ray_cast(state, ray, &hit_record);
            
            if (has_hit(hit_record))
            {
                Material mat = scene->materials[hit_record.mat_index];
				
                ray.origin = hit_record.hit_point;
				
                // Decide if we want to refract or reflect
                // Check if refraction_probability != 0 (it is OK to compare floats against constants)
                if ((mat.refraction_probability != 0.0f)) //&& (random_unitlateral(&series) <= mat.refraction_probability))
                {
                    Vec3 reflected = reflect(ray.direction, hit_record.normal);
                    // Coefficient in Snells law between glass and air
                    f32 ref_idx = 1.5f;
                    // sin(Theta0) / sin(Theta1) - different when light goes in or out of glass
                    f32 refract_coef;
                    f32 cos_atten;
                    Vec3 actual_normal;
                    if (dot(ray.direction, hit_record.normal) > 0)
                    {
                        refract_coef = ref_idx;
                        cos_atten = ref_idx * dot(ray.direction, hit_record.normal);
                        actual_normal = vec3_neg(hit_record.normal);
                    }
                    else
                    {
                        actual_normal = hit_record.normal;
                        refract_coef = 1.0f / ref_idx;
                        cos_atten = -dot(ray.direction, hit_record.normal);
                    }
					
                    f32 reflect_prob;
                    f32 dt = dot(ray.direction, actual_normal);
                    f32 discriminant = 1.0f - refract_coef * refract_coef * (1.0f - dt * dt);
                    Vec3 refracted;
					
                    if (discriminant > 0.0f)
                    {
                        refracted = vec3_sub(vec3_muls(vec3_sub(ray.direction, vec3_muls(actual_normal, dt)), refract_coef), vec3_muls(actual_normal, sqrt32(discriminant)));
                        reflect_prob = schlick(cos_atten, ref_idx);
                    }
                    else
                    {
                        reflect_prob = 1.0f;
                    }
					
					Vec3 new_dir;
                    if (random_unitlateral(&series) <= reflect_prob)
                    {
                        new_dir = normalize(reflected);
                    }
                    else
                    {
                        new_dir = normalize(refracted);
                    }
					ray_update_dir(&ray, new_dir);
                }
                else
                {
                    ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, mat.emit_color));
                    f32 cos_atten = dot(ray.inverse_direction, hit_record.normal);
                    if (cos_atten < 0.0f)
                    {
                        cos_atten = 0.0f;
                    }
                    
					Texture *texture = mat.texture;
                    Vec3 hit_color = sample_texture(texture, hit_record.uv, hit_record.hit_point);
					
                    hit_color = vec3_muls(hit_color, cos_atten);
                    // Attenuation is updated via multiplying it with current ray hit color
                    // This way, each hit surface color is added to the resulting color 
                    attenuation = vec3_mul(attenuation, hit_color);
                    
					Vec3 pure_reflection = reflect(ray.direction, hit_record.normal);
                    Vec3 scattered_reflection = normalize(vec3_add(hit_record.normal, random_unit_sphere(&series)));
					Vec3 reflection = normalize(vec3_lerp(scattered_reflection, pure_reflection, mat.scatter));
					ray_update_dir(&ray, reflection);
                }
            }
            else
            {
                Vec3 background_color = scene->materials[0].emit_color;
                // ray_cast_color = background_color;
                ray_cast_color = vec3_add(ray_cast_color, vec3_mul(attenuation, background_color));
                break;
            }
        }
        
        final_color = vec3_add(final_color, vec3_muls(ray_cast_color, contrib));
    }
	
    final_color = vec3_muls(vec3(linear1_to_srgb1(final_color.x),
                                 linear1_to_srgb1(final_color.y),
                                 linear1_to_srgb1(final_color.z)),
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
	
    RenderWorkOrder *order = queue->work_orders + work_order_index;
    u32 xmin = order->xmin;
    u32 ymin = order->ymin;
    u32 one_past_xmax = order->one_past_xmax;
    u32 one_past_ymax = order->one_past_ymax;
    
    ImageU32 *image = order->image;
    
    CastState state = {0};
    state.scene = order->scene;
    state.series = order->random_series;
    state.rays_per_pixel = queue->rays_per_pixel;
    state.max_bounce_count = queue->max_bounce_count;
	
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

TriangleMesh 
triangle_mesh(u32 nfaces, u32 *fi, u32 *vi, Vec3 *p, Vec3 *n, Vec2 *st)
{
    // @TODO(hl): Use single malloc for all allocations to avoid heap fragmentation or whatever.
    TriangleMesh result = {0};
    
    u32 k = 0, max_vertex_index = 0;
    for (u32 i = 0;
         i < nfaces;
         ++i)
    {
        result.triangle_count += fi[i] - 2;
        for (u32 j = 0;
             j < fi[i];
             ++j)
        {
            if (vi[k + j] > max_vertex_index)
            {
                max_vertex_index = vi[k + j];
            }
        }
        k += fi[i];
    }
    ++max_vertex_index;
    result.max_vertex_index = max_vertex_index;
    
    result.p = calloc(max_vertex_index, sizeof(Vec3));
    memcpy(result.p, p, max_vertex_index * sizeof(Vec3));
    
    for (u32 plane_normal_index = 0;
         plane_normal_index < BVH_NUM_PLANE_SET_NORMALS;
         ++plane_normal_index)
    {
        f32 dmin = F32_MAX;
        f32 dmax = F32_MIN;
        
        for (u32 vertex_index = 0;
             vertex_index < result.max_vertex_index;
             ++vertex_index)
        {
            f32 d = dot(plane_set_normals[plane_normal_index], result.p[vertex_index]);
            if (d < dmin)
            {
                dmin = d;
            }
            if (d > dmax)
            {
                dmax = d;
            }
        }
        
        result.bvh.extents.dmin[plane_normal_index] = dmin;
        result.bvh.extents.dmax[plane_normal_index] = dmax;
    }
    
    result.n = calloc(max_vertex_index, sizeof(Vec3));
    memcpy(result.n, n, max_vertex_index * sizeof(Vec3));
    
    result.uvs = calloc(max_vertex_index, sizeof(Vec2));
    memcpy(result.uvs, st, max_vertex_index * sizeof(Vec2));
    
    result.tri_indices = calloc(result.triangle_count * 3, sizeof(u32));
    u32 l = 0;
    for (u32 i = 0, k = 0; 
         i < nfaces;
         ++i)
    {
        for (u32 j = 0; 
             j < fi[i] - 2;
             ++j)
        {
            result.tri_indices[l] = vi[k];
            result.tri_indices[l + 1] = vi[k + j + 1];
            result.tri_indices[l + 2] = vi[k + j + 2];
            l += 3;
        }
        k += fi[i];
    }
    
    return result;
}

TriangleMesh
make_poly_sphere(f32 r, u32 divs)
{
    // generate points                                                                                                                                                                                      
    u32 numVertices = (divs - 1) * divs + 2; 
    // @TODO(hl): Memory pool!
    Vec3 *P = calloc(numVertices, sizeof(Vec3)); 
    Vec3 *N = calloc(numVertices, sizeof(Vec3)); 
    Vec2 *st = calloc(numVertices, sizeof(Vec2)); 
	
    float u = -HALF_PI; 
    float v = -PI; 
    float du = PI / divs; 
    float dv = 2 * PI / divs; 
	
    P[0] = N[0] = vec3(0, 0, -r); 
    u32 k = 1; 
    for (u32 i = 0;
         i < divs - 1;
         i++) { 
        u += du; 
        v = -PI; 
        for (u32 j = 0; 
             j < divs;
             j++) { 
            float x = r * cos32(u) * cos32(v); 
            float z = r * sin32(u); 
            float y = r * cos32(u) * sin32(v) ; 
            P[k] = N[k] = vec3(x, y, z); 
            st[k].x = u / PI + 0.5; 
            st[k].y = v * 0.5 / PI + 0.5; 
            v += dv, k++; 
        } 
    } 
    P[k] = N[k] = vec3(0, 0, r); 
	
    u32 npolys = divs * divs; 
    u32 *faceIndex = calloc(npolys, sizeof(u32)); 
    u32 *vertsIndex = calloc((6 + (divs - 1) * 4) * divs, sizeof(u32)); 
	
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
    
    return triangle_mesh(npolys, faceIndex, vertsIndex, P, N, st); 
}

void 
make_utah_teapot(TriangleMesh *meshes) 
{ 
    uint32_t divs = 8; 
    Vec3 *P = calloc((divs + 1) * (divs + 1), sizeof(Vec3)); 
    Vec3 *N = calloc((divs + 1) * (divs + 1), sizeof(Vec3)); 
    Vec2 *st = calloc((divs + 1) * (divs + 1), sizeof(Vec2)); 
    u32 *nvertices = calloc(divs * divs, sizeof(u32));
    u32 *vertices = calloc(divs * divs * 4, sizeof(u32));
	
    // face connectivity - all patches are subdivided the same way so there
    // share the same topology and uvs
    for (u32 j = 0, k = 0; j < divs; ++j) { 
        for (u32 i = 0; i < divs; ++i, ++k) { 
            nvertices[k] = 4; 
            vertices[k * 4    ] = (divs + 1) * j + i; 
            vertices[k * 4 + 1] = (divs + 1) * j + i + 1; 
            vertices[k * 4 + 2] = (divs + 1) * (j + 1) + i + 1; 
            vertices[k * 4 + 3] = (divs + 1) * (j + 1) + i; 
        } 
    } 
	
    Vec3 controlPoints[16]; 
    for (int np = 0; np < UTAH_TEAPOT_NUM_PATCHES; ++np)  // kTeapotNumPatches 
    {
        // for (u32 np = 0; np < kTeapotNumPatches; ++np) { // kTeapotNumPatches 
        // set the control points for the current patch
        for (uint32_t i = 0; i < 16; ++i) 
            controlPoints[i].e[0] = utah_teapot_vertices[utah_teapot_patches[np][i] - 1][0], 
		controlPoints[i].e[1] = utah_teapot_vertices[utah_teapot_patches[np][i] - 1][1], 
		controlPoints[i].e[2] = utah_teapot_vertices[utah_teapot_patches[np][i] - 1][2]; 
		
        // generate grid
        for (uint16_t j = 0, k = 0; j <= divs; ++j) { 
            float v = j / (float)divs; 
            for (uint16_t i = 0; i <= divs; ++i, ++k) { 
                float u = i / (float)divs; 
                P[k] = eval_bezier_patch(controlPoints, u, v); 
                Vec3 dU = deirv_u_bezier(controlPoints, u, v); 
                Vec3 dV = deriv_v_bezier(controlPoints, u, v); 
                N[k] = normalize(cross(dU, dV)); 
                st[k].x = u; 
                st[k].y = v; 
            } 
        } 
        
        for (u32 i = 0; i < (divs + 1) * (divs + 1); ++i)
        {
            P[i].z -= 5.0f;
        }
        
        meshes[np] = triangle_mesh(divs * divs, nvertices, vertices, P, N, st);
    }
} 

// Returns scene with same objects in it
// Can be used for testing and comparing results
void 
init_sample_scene(Scene *scene, ImageU32 *image)
{
	u32 texture_pool_size = MEGABYTES(1);
	MemoryPool texture_pool = memory_pool(malloc(texture_pool_size), texture_pool_size);
	
    Texture *textures = memory_pool_alloc_array(&texture_pool, Texture, 100);
	
    texture_init_solid(textures + 0, vec3(0.5, 0.5, 0.5));
    texture_init_solid(textures + 1, vec3(0.2, 0.3, 0.1));
    texture_init_checkered(textures + 2, textures + 0, textures + 1);
    texture_init_solid(textures + 3, vec3(0.7, 0.5, 0.3));
	texture_init_image(textures + 4, "data/earthmap.jpg");
    texture_init_solid(textures + 5, vec3(0.4f, 0.8f, 0.9f));
    texture_init_solid(textures + 6, vec3(0.95f, 0.95f, 0.95f));
    texture_init_solid(textures + 7, vec3(0.75f, 0.33f, 0.75f));
    texture_init_solid(textures + 8, vec3(0.75f, 0.33f, 0.33f));
    texture_init_solid(textures + 9, vec3(0.8f, 0.8f, 0.1f));
    texture_init_solid(textures + 10, vec3(0.8f, 0.1f, 0.8f));
	
    
    // @NOTE(hl): Don't malloc arrays because scene settings is always the same
    static Material materials[11] = {0};
    materials[0].emit_color = vec3(0.3f, 0.4f, 0.5f);
    materials[1].texture = textures + 2;
    materials[2].texture = textures + 3;
    materials[2].refraction_probability = 1.0f;
    materials[3].emit_color = vec3(15.0f, 15.0f, 15.0f);
    materials[4].texture = textures + 4;
    materials[5].texture = textures + 5;
    materials[5].scatter = 0.85f;
    materials[6].texture = textures + 6;
    materials[6].scatter = 0.99f;
    materials[7].texture = textures + 7;
    materials[8].texture = textures + 8;
    materials[9].texture = textures + 9;
    materials[10].texture = textures + 10;
    
    static Object objects[100] = {0};
    Object *current_object = objects;
    object_init_plane(current_object++, empty_transform(), (Plane) {
        .normal = vec3(0, 0, 1),
        .dist = 0,
    }, 1);
    object_init_sphere(current_object++, empty_transform(), (Sphere) {
        .pos = vec3(-3, 2, 0),
        .radius = 2.0f
    }, 6);
    object_init_sphere(current_object++, empty_transform(), (Sphere) {
        .pos = vec3(3, -2, 0),
        .radius = 1.0f
    }, 3);
    object_init_sphere(current_object++, empty_transform(), (Sphere) {
        .pos = vec3(-2, -1, 2),
        .radius = 1.0f
    }, 4);
    object_init_sphere(current_object++, empty_transform(), (Sphere) {
        .pos = vec3(1, -1, 3),
        .radius = 1.0f
    }, 5);
    object_init_sphere(current_object++, empty_transform(), (Sphere) {
        .pos = vec3(1, -3, 1),
        .radius = 1.0f
    }, 2);
    object_init_disk(current_object++, empty_transform(), (Disk) {
        .point = vec3(-2, -5, 0.1f),
        .normal = vec3(0, 0, 1),
        .radius = 1.0f
    }, 8);    
    scene->objects = objects;
    scene->object_count = current_object - objects;
       
    scene->camera = make_camera(vec3_muls(vec3(-1.5, -10, 1.5), 1.5f), image);
	
    scene->material_count = array_size(materials);
    scene->materials = materials;
}

#if 0 
void 
write_yz_rect(Triangle *t, Vec2 yz0, Vec2 yz1, f32 x, u32 mat_index)
{
    Vec3 v00 = vec3(x, yz0.x, yz0.y);
    Vec3 v01 = vec3(x, yz0.x, yz1.y);
    Vec3 v10 = vec3(x, yz1.x, yz0.y);
    Vec3 v11 = vec3(x, yz1.x, yz1.y);
    
    Vec3 normal = normalize(cross(vec3_sub(v01, v00), vec3_sub(v10, v00)));
    
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v01,
        .vertex2 = v11,
        .normal = normal,
        .mat_index = mat_index
    };
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v11,
        .vertex2 = v10,
        .normal = normal,
        .mat_index = mat_index
    };
}

void 
write_xy_rect(Triangle *t, Vec2 xy0, Vec2 xy1, f32 z, u32 mat_index)
{
    Vec3 v00 = vec3(xy0.x, xy0.y, z);
    Vec3 v01 = vec3(xy0.x, xy1.y, z);
    Vec3 v10 = vec3(xy1.x, xy0.y, z);
    Vec3 v11 = vec3(xy1.x, xy1.y, z);
    
    Vec3 normal = normalize(cross(vec3_sub(v01, v00), vec3_sub(v10, v00)));
    
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v01,
        .vertex2 = v11,
        .normal = normal,
        .mat_index = mat_index
    };
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v11,
        .vertex2 = v10,
        .normal = normal,
        .mat_index = mat_index
    };
}

void 
write_xz_rect(Triangle *t, Vec2 xz0, Vec2 xz1, f32 y, u32 mat_index)
{
    Vec3 v00 = vec3(xz0.x, y, xz0.y);
    Vec3 v01 = vec3(xz0.x, y, xz1.y);
    Vec3 v10 = vec3(xz1.x, y, xz0.y);
    Vec3 v11 = vec3(xz1.x, y, xz1.y);
    
    Vec3 normal = normalize(cross(vec3_sub(v01, v00), vec3_sub(v10, v00)));
    
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v01,
        .vertex2 = v11,
        .normal = normal,
        .mat_index = mat_index
    };
    *t++ = (Triangle) {
        .vertex0 = v00,
        .vertex1 = v11,
        .vertex2 = v10,
        .normal = normal,
        .mat_index = mat_index
    };
}

void 
make_cornell_box(Scene *scene, ImageU32 *image)
{
    static Material materials[9] = {0};
    // materials[0].emit_color = vec3_muls(vec3(0.3f, 0.4f, 0.5f), 5);
    // materials[0].emit_color = vec3(.5f, .5f, .5f);
    // materials[1].texture = texture_solid_color(vec3(0.65f, 0.05f, 0.05f));
    // materials[2].texture = texture_solid_color(vec3(0.73f, 0.73f, 0.73f));
    // materials[3].texture = texture_solid_color(vec3(0.12f, 0.45f, 0.15f));
    // f32 c = 30.0f;
    // materials[4].emit_color = vec3(c, c, c * 0.8f);
    // materials[5].texture = texture_image("e:\\dev\\ray\\data\\pano.jpg");
    // materials[6].texture = texture_solid_color(vec3(0.7f, 0.5f, 0.3f));
    // materials[6].refraction_probability = 1.0f;
    // materials[7].texture = texture_solid_color(vec3(0.8f, 0.8f, 0.1f));
    // materials[8].texture = texture_solid_color(vec3(0.3f, 0.3f, 0.3f));
	
    // static Plane planes[1] = {0};
    // planes[0] = (Plane) {
    //     .normal = vec3(0, 0, 1),
    //     .mat_index = 8,  
    //     .dist = 0
    // };
    // scene->planes = planes;
    // scene->plane_count = array_size(planes);
	
    static Triangle triangles[12] = {0};
    write_yz_rect(triangles, vec2(-5, -5), vec2(5, 5), -5, 1);
    write_yz_rect(triangles + 2, vec2(-5, -5), vec2(5, 5),  5, 3);
    f32 d = 1.2f;
    write_xy_rect(triangles + 4, vec2(-d, -d), vec2(d, d),  4.99f, 4);
    write_xy_rect(triangles + 6, vec2(-5, -5), vec2(5, 5),  5.0f,  2);
    write_xy_rect(triangles + 8, vec2(-5, -5), vec2(5, 5), -5.0f,  2);
    write_xz_rect(triangles + 10, vec2(-5, -5), vec2(5, 5), 5.0f,  2);
    scene->triangles = triangles;
    scene->triangle_count = array_size(triangles);
    
    static TriangleMesh meshes[UTAH_TEAPOT_NUM_PATCHES] = { 0 };
    make_utah_teapot(meshes);
    for (u32 i = 0; i < UTAH_TEAPOT_NUM_PATCHES; ++i) meshes[i].mat_index = 2;
    scene->mesh_count = array_size(meshes);
    scene->meshes = meshes;
    
    // static Sphere spheres[2] = {0};
    // spheres[0] = (Sphere) {
    //     .mat_index = 6,
    //     .pos = vec3(1.4f, -2.3f, -3.5f),
    //     .radius = 1.5f
    // };
    // spheres[1] = (Sphere) {
    //     .mat_index = 5,
    //     .pos = vec3(-1.25f, 2.0f, -2.0f),
    //     .radius = 3.0f
    // };
    // scene->sphere_count = array_size(spheres);
    // scene->spheres = spheres;
    
    
    
    // Box3 box0 = { 
    //     .min = vec3(0.7f, -3.3f, -5.0f),
    //     .max = vec3(3.5f, -0.4f, -2.0f)
    // };
    // add_box(rects + 6, box0, 2,  0.261799f * 1.3f);
    // Box3 box1 = {
    //     .min = vec3(-2.75f, 0.3f, -5.0f),
    //     .max = vec3(  0.2f, 3.3f,  0.95f)
    // };
    // add_box(rects + 12, box1, 2, -0.314159265f);
    
    // scene->camera = camera(vec3(0, -16, 4), image);
    scene->camera = make_camera(vec3(0, -16, 0), image);
	
    scene->material_count = array_size(materials);
    scene->materials = materials;
}
#endif 
static void 
parse_command_line_arguments(RaySettings *settings, u32 argc, char **argv)
{
    if (argc == 1)
    {
        return;
    }
    
    u32 argument_count = argc;
    char **arguments = argv;
    u32 cursor = 1;
    
    while (cursor < argument_count)
    {
        char *arg = arguments[cursor];
        
        if (!strcmp(arg, "-help"))
        {
            printf("OVERVIEW: command-line raytracer\n\n");
            printf("USAGE: ray.exe [options]\n\n");
            printf("OPTIONS:\n");
#define option_format "  %-15s %s\n"
            printf(option_format, "-help", "prints help");
            printf(option_format, "-out <path>", "specifies name of output image (.png)");
            printf(option_format, "-size <w> <h>", "specified size of output image");
            printf(option_format, "-scene <path>", "specifies scene. If not set, use default");
            printf(option_format, "-rpp <n>", "speciifes Rays Per Pixel. Default is 128");
            printf(option_format, "-mbc <n>", "speciifes Max Bounce Count. Default is 8");
            printf(option_format, "-open", "open output image after done");
            
            exit(0);
        }
        if (!strcmp(arg, "-out"))
        {
            if (cursor + 1 >= argument_count)
            {
                fprintf(stderr, "[ERROR] Argument -out takes exactly 1 position argument\n");
                break;
            }
            
            char *out_file = arguments[cursor + 1];
            settings->output_filename = out_file;
            
            cursor += 2;
        }
        else if (!strcmp(arg, "-size"))
        {
            if (cursor + 2 >= argument_count)
            {
                fprintf(stderr, "[ERROR] Argument -size takes exactly 2 position arguments\n");
                break;
            }
            
            u32 width = atoi(arguments[cursor + 1]);
            u32 height = atoi(arguments[cursor + 2]);
            settings->output_width = width;
            settings->output_height = height;
            
            cursor += 3;
        }
        else if (!strcmp(arg, "-scene"))
        {
            if (cursor + 1 >= argument_count)
            {
                fprintf(stderr, "[ERROR] Argument -scene takes exactly 1 position argument\n");
                break;
            }
            
            char *scene = arguments[cursor + 1];
            settings->input_scene = scene;
            
            cursor += 2;
        }
        else if (!strcmp(arg, "-rpp"))
        {
            if (cursor + 1 >= argument_count)
            {
                fprintf(stderr, "[ERROR] Argument -rpp takes exactly 1 position argument\n");
                break;
            }
            
            u32 value = atoi(arguments[cursor + 1]);
            settings->rays_per_pixel = value;
            
            cursor += 2;
        }
        else if (!strcmp(arg, "-mbc"))
        {
            if (cursor + 1 >= argument_count)
            {
                fprintf(stderr, "[ERROR] Argument -mbc takes exactly 1 position argument\n");
                break;
            }
            
            u32 value = atoi(arguments[cursor + 1]);
            settings->max_bounce_count = value;
            
            cursor += 2;
        }
        else if (!strcmp(arg, "-open"))
        {
            settings->open_image_after_done = true;
            ++cursor;
        }
        else 
        {
            fprintf(stderr, "[ERROR] Unexpected argument '%s'!\n", arg);
            break;
        }
    }
}

int main(int argc, char **argv)
{
    printf("Ray started!\n");
    
    RaySettings settings = {0};
    settings.output_filename = "out.png";
    settings.output_width = 600;
    settings.output_height = 600;
    settings.rays_per_pixel = 128;
    settings.max_bounce_count = 8;
    parse_command_line_arguments(&settings, argc, argv);
    printf("SETTINGS:\n");
    printf("Output image: %s (%ux%u)\n", settings.output_filename, settings.output_width, settings.output_height);
    printf("Scene: %s", settings.input_scene ? : "default\n");
    printf("Rays per pixel: %u, Max bounce count: %u\n", settings.rays_per_pixel, settings.max_bounce_count);
    printf("\n");
    
    // Raycasting output is simply an image
    ImageU32 image = {0};
    image_u32_init(&image, settings.output_width, settings.output_height);
	
    // Initialize scene
    Scene scene = {0};
	// make_test_scene(&scene, &image);
	init_sample_scene(&scene, &image);
	// init_sample_scene(&scene, &image);
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
    queue.rays_per_pixel = settings.rays_per_pixel;
    queue.max_bounce_count = settings.max_bounce_count;
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
            order->random_series = (RandomSeries){tile_x * 12098 + tile_y * 23771 +
					tile_count_x * 29103 + tile_count_y * 34298};
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
    char *filename = settings.output_filename;
    image_u32_save(&image, filename);
	
    if (settings.open_image_after_done)
    {
        // @TODO(hl): Make multi-platform
        char command[256];
        format_string(command, sizeof(command), "start %s", settings.output_filename);
        system(command);
    }
    
	
    printf("\nRay finished!\n");
    return 0;
}