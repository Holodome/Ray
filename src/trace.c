#include "trace.h"

static bool 
is_black(Vec3 color) {
    return !(isfinite(color.x) && isfinite(color.y) && isfinite(color.z));
}

static Vec3 
align_to_direction(Vec3 n, f32 cos_theta, f32 phi) {
    f32 sin_theta = sqrt32(saturate(1.0 - cos_theta * cos_theta));
    
    ONB onb = onb_from_w(n);
    return v3add(v3muls(v3add(v3muls(onb.u, cosf(phi)),
                              v3muls(onb.v, sinf(phi))), 
                        sin_theta), 
                 v3muls(n, cos_theta));
}

static Vec3 
sample_cosine_weighted_hemisphere(RandomSeries *entropy, Vec3 n) {
    f32 r0 = randomu(entropy);
    f32 r1 = randomu(entropy);
    f32 cos_theta = sqrt32(r0);
    return align_to_direction(n, cos_theta, r1 * TWO_PI);
}

static Vec3  
sample_ggx_distribution(RandomSeries *entropy, Vec3 n, f32 alpha_sq) {
    f32 r0 = randomu(entropy);
    f32 r1 = randomu(entropy);
    f32 cos_theta = sqrt32(saturate((1.0 - r0) / (r0 * (alpha_sq - 1.0) + 1.0)));
    return align_to_direction(n, cos_theta, r1 * TWO_PI);
}

static f32
ggx_normal_distribution(f32 alpha_sq, Vec3 n, Vec3 m) {
    f32 cos_theta = dot(n, m);
    f32 cos_theta_sq = sq(cos_theta);
    f32 denom = cos_theta_sq * (alpha_sq - 1.0) + 1.0;
    f32 result = 0;
    if (cos_theta > 0) {
        result = alpha_sq / (PI * sq(denom));
    }
    return result;
}

static f32 
ggx_visibility(f32 alpha_sq, Vec3 w, Vec3 n, Vec3 m) {
    f32 ndotw = dot(n, w);
    f32 mdotw = dot(m, w);
    if (mdotw * ndotw <= 0.0) {
        return 0;
    }
    f32 cos_theta_sq = sq(ndotw);
    f32 tan_theta_sq = (1.0 - cos_theta_sq) / cos_theta_sq;
    if (tan_theta_sq == 0.0) {
        return 1;
    }
    return 2.0 / (1.0 + sqrt32(1.0 + alpha_sq * tan_theta_sq));
}

static f32 
ggx_visibility_term(f32 alpha_sq, Vec3 wi, Vec3 wo, Vec3 n, Vec3 m) {
    return ggx_visibility(alpha_sq, wi, n, m) * ggx_visibility(alpha_sq, wo, n, m);
}

static f32 
fresnel_dielectric(Vec3 i, Vec3 m, f32 eta) {
    f32 result = 1.0;
    f32 cos_theta_i = abs32(dot(i, m));
    f32 sin_theta_o_sq = (eta * eta) * (1.0 - cos_theta_i * cos_theta_i);
    if (sin_theta_o_sq <= 1.0) {
        f32 cos_theta_o = sqrt32(saturate(1.0f - sin_theta_o_sq));
        f32 rs = (cos_theta_i - eta * cos_theta_o) / (cos_theta_i + eta * cos_theta_o);
        f32 rp = (eta * cos_theta_i - cos_theta_o) / (eta * cos_theta_i + cos_theta_o);
        result = 0.5 * (rs * rs + rp * rp);
    }
    return result;
}

static f32 
remap_roughness(f32 r, f32 ndoti) {
    f32 alpha = (1.2 - 0.2 * sqrt32(abs32(ndoti))) * r;
    return sq(alpha);
}

static bool 
bounds3_hit(Bounds3 bounds, Ray ray, f32 t_min, f32 t_max) {
    for (u32 a = 0;
         a < 3;
         ++a) {
        f32 inv_d = 1.0f / ray.dir.e[a];
        f32 t0 = (bounds.min.e[a] - ray.orig.e[a]) * inv_d;
        f32 t1 = (bounds.max.e[a] - ray.orig.e[a]) * inv_d;
        if (inv_d < 0.0f) {
            f32 temp = t0;
            t0 = t1;
            t1 = temp;
        }
        
        t_min = max32(t0, t_min);
        t_max = min32(t1, t_max);
        
        if (t_max < t_min) {
            return false;
        }
    }
    return true;
}

static bool 
triangle_hit(Vec3 p0, Vec3 p1, Vec3 p2, Ray ray, f32 *td, f32 *ud, f32 *vd, RayCastStatistics *stats) {
    ++stats->ray_triangle_collision_tests;
    bool result = false;
     
    Vec3 e1 = v3sub(p1, p0);
    Vec3 e2 = v3sub(p2, p0);
    Vec3 h = cross(ray.dir, e2);
    f32 a = dot(e1, h);
    
    if ((a < -0.001f) || (a > 0.001f)) {
        f32 f = 1.0f / a;
        Vec3 s = v3sub(ray.orig, p0);
        f32 u = f * dot(s, h);
        Vec3 q = cross(s, e1);
        f32 v = f * dot(ray.dir, q);
        f32 t = f * dot(e2, q);
        if ((0 < u) && (u < 1) && (v > 0) && (u + v < 1)) {
            *td = t;
            *ud = u;
            *vd = v;
            
            result = true;
        }
    }
    
    stats->ray_triangle_collision_test_succeses += result;
    return result;
}

static f32 
triangle_area(Vec3 p0, Vec3 p1, Vec3 p2) {
    return 0.5f * length(cross(v3sub(p1, p0), v3sub(p2, p0)));
}

#define BOX_COMPARATOR_SIGNATURE(_name) int _name(void *w, const void *a, const void *b)
typedef BOX_COMPARATOR_SIGNATURE(BoxComparator);

static int 
bounds3_compare(World *world, ObjectHandle a, ObjectHandle b, u32 axis) {
    Bounds3 b0 = get_object_bounds(world, a);
    Bounds3 b1 = get_object_bounds(world, b);
    return b0.min.e[axis] < b1.min.e[axis];
}

static BOX_COMPARATOR_SIGNATURE(bounds3_compare_x) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 0); }
static BOX_COMPARATOR_SIGNATURE(bounds3_compare_y) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 1); }
static BOX_COMPARATOR_SIGNATURE(bounds3_compare_z) { return bounds3_compare((World *)w, *((ObjectHandle *)a), *((ObjectHandle *)b), 2); }

static Vec3 
reflect(Vec3 v, Vec3 n) {
    Vec3 result = v3sub(v, v3muls(n, 2.0f * dot(v, n)));
    return result;
}

static Vec3 
refract(Vec3 v, Vec3 n, f32 etai_over_etat) {
    f32 cos_theta = min32(-dot(v, n), 1.0f);
    Vec3 r_out_perp = v3muls(v3add(v, v3muls(n, cos_theta)), etai_over_etat);
    Vec3 r_out_parallel = v3muls(n, -sqrt32(abs32(1.0f - length_sq(r_out_perp))));
    Vec3 result = normalize(v3add(r_out_perp, r_out_parallel));
    return result; 
}

static void 
sphere_get_uv(Vec3 p, f32 *u, f32 *v) {
    f32 theta = acosf(-p.y);
    f32 phi = atan2f(-p.z, p.x) + PI;
    *u = phi / TWO_PI;
    *v = theta / PI;
}

static Bounds3 
transform_bounds(Bounds3 bi, Mat4x4 t) {
    Bounds3 bounds = bounds3i(mat4x4_mul_vec3(t, bi.min));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.min.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.max.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.min.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.min.x, bi.max.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.max.y, bi.min.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.min.y, bi.max.z)));
    bounds = bounds3_extend(bounds, mat4x4_mul_vec3(t, v3(bi.max.x, bi.max.y, bi.max.z)));
    return bounds;
}

void 
hit_set_normal(HitRecord *hrec, Vec3 n, Ray ray) {
    hrec->ndoti = hrec->ndotio = dot(ray.dir, n);
    hrec->no = n;
    if (dot(ray.dir, n) <= 0.0f) {
        hrec->is_front_face = true;
        hrec->n = n;
    } else {
        hrec->is_front_face = false;
        hrec->n = v3neg(n);
        hrec->ndoti = -hrec->ndoti;
    }
}

Vec3
sample_texture(World *world, TextureHandle handle, HitRecord *hrec) {
    Vec3 result;
    
    Texture *texture = get_texture(world, handle); 
    switch(texture->type) {
        case TextureType_Solid: {
            result = texture->solid.c;
        } break;
        case TextureType_Checkerboard: {
            if (((i32)floorf(hrec->u) + (i32)floorf(hrec->v)) % 2 == 0) {
                result = sample_texture(world, texture->checkerboard.t1, hrec);
            } else {
                result = sample_texture(world, texture->checkerboard.t2, hrec);
            }
        } break;
        case TextureType_Checkerboard3D: {
            if (((i32)floorf(hrec->p.x) + (i32)floorf(hrec->p.y) + (i32)floorf(hrec->p.z)) % 2 == 0) {
                result = sample_texture(world, texture->checkerboard.t1, hrec);
            } else {
                result = sample_texture(world, texture->checkerboard.t2, hrec);
            }
        } break;
        case TextureType_Image: {
            f32 u = clamp(hrec->u, 0, 1);
            f32 v = clamp(hrec->v, 0, 1);
            
            u32 x = roundf(u * texture->image.i.w);
            u32 y = roundf(v * texture->image.i.h);
            
            if (x >= texture->image.i.w) {
                x = texture->image.i.w - 1;
            }
            if (y >= texture->image.i.h) {
                y = texture->image.i.h - 1;
            }
            
            f32 color_scale = 1.0f / 255.0f;
            u8 *pixel = (u8 *)image_get_pixel_pointer(&texture->image.i, x, y);
            
            result = v3(color_scale * pixel[0], color_scale * pixel[1], color_scale * pixel[2]);
        } break;
        case TextureType_Perlin: {
            // f32 noise = perlin_turb(&texture->perlin.p, v3muls(hrec->p, texture->perlin.s), 7);
            f32 noise = 0.5f * (1 + sinf(texture->perlin.s * hrec->p.z + 10 * perlin_turb(&texture->perlin.p, hrec->p, 7)));
            result = v3muls(v3s(1), noise);
        } break;
        case TextureType_UV: {
            result = v3(hrec->u, hrec->v, 0);
        } break;
        case TextureType_Normal: {
            result = v3muls(v3add(hrec->n, v3s(1)), 0.5f);
        } break;
        INVALID_DEFAULT_CASE;
    }
    return result;
} 

bool
material_scatter(World *world, Ray ray, HitRecord hrec, RayCastData data, ScatterRecord *srec) {
    bool result = false;
    Vec3 no = hrec.n;
    Vec3 wi = ray.dir;

    Material *mat = get_material(world, hrec.mat);
    switch(mat->type) {
        case MaterialType_Lambertian: {
            srec->dir = sample_cosine_weighted_hemisphere(data.entropy, no);
            result = true;
        } break;
        case MaterialType_Metal: {
            f32 alpha_sq = sq(mat->roughness);
            Vec3 microfacet_n = sample_ggx_distribution(data.entropy, no, alpha_sq);
            srec->dir = reflect(wi, microfacet_n);
            result = true;
        } break;
        case MaterialType_Plastic: {
            Vec3 m = sample_ggx_distribution(data.entropy, no, sq(mat->roughness));
            if (randomu(data.entropy) < fresnel_dielectric(wi, m, mat->ext_ior / mat->int_ior)) {
                srec->dir = sample_cosine_weighted_hemisphere(data.entropy, no);
            } else {
                srec->dir = reflect(wi, m);
            }
            result = true;
        } break;
        case MaterialType_Dielectric: {
            Vec3 n = hrec.no;
            f32 eta = mat->int_ior / mat->ext_ior;
            if (hrec.is_front_face) {
                eta = 1.0f / eta;
            }
            f32 ndoti = -hrec.ndotio;
            f32 a = remap_roughness(mat->roughness, ndoti);
               
            Vec3 microfacet = sample_ggx_distribution(data.entropy, n, a);
            if (randomu(data.entropy) > fresnel_dielectric(wi, microfacet, eta)) {
                srec->dir = refract(wi, no, eta);
                // assert(dot(n, srec->dir) * dot(n, wi) >= 0.0);
            } else {
                srec->dir = reflect(wi, microfacet);
                // assert(dot(n, srec->dir) * dot(n, wi) <= 0.0);
            }
            result = true;
        } break;
        case MaterialType_Isotropic: {
            srec->dir = random_unit_sphere(data.entropy);
            result = true;
        } break;
        case MaterialType_DiffuseLight: {
            
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;   
}

bool 
material_compute_scattering_functions(World *world, Vec3 wi, Vec3 no, HitRecord hrec, 
                                      ScatterRecord *srec, RayCastData data) {
    Material *mat = get_material(world, hrec.mat);
    
    Vec3 wo = srec->dir;
    switch (mat->type) {
        case MaterialType_Lambertian: {
            f32 ndoto = dot(no, wo);
            f32 ndoti = -dot(no, wi);
            if (ndoto > 0 && ndoti > 0) {
                Vec3 diffuse = sample_texture(world, mat->diffuse, &hrec);
                srec->bsdf = v3muls(diffuse, ndoto * INV_PI);
                srec->pdf = ndoto * INV_PI;
                srec->weight = diffuse;
            }
        } break;
        case MaterialType_Metal: {
            f32 alpha_sq = sq(mat->roughness);
            f32 ndoto = dot(no, wo);
            f32 ndoti = -dot(no, wi);
            
            if ((ndoto * ndoti > 0.0) && (ndoto > 0.0) && (ndoti > 0.0)) {
                Vec3 m = normalize(v3sub(wo, wi));
                f32 ndotm = dot(no, m);
                f32 mdoto = dot(m, wo);
                
                f32 a = alpha_sq;
                f32 d = ggx_normal_distribution(a, no, m);
                f32 g = ggx_visibility_term(a, wi, wo, no, m);
                f32 j = 1.0 / (4.0 * mdoto);
                
                f32 f = 1.0;
                Vec3 specular = sample_texture(world, mat->specular, &hrec);
                srec->bsdf = v3muls(specular, f * (d * g / (4.0 * ndoti)));
                srec->pdf = d * ndotm * j;
                srec->weight = v3muls(specular, f * (g * mdoto / (ndotm * ndoti)));
            }
        } break;
        case MaterialType_Plastic: {
            f32 ndoti = -dot(no, wi);
            f32 ndoto = dot(no, wo);
            if ((ndoto * ndoti > 0.0) && (ndoto > 0.0) && (ndoti > 0.0)) {
                Vec3 m = normalize(v3sub(wo, wi));
                f32 ndotm = dot(no, m);
                f32 mdoto = dot(m, wo);
                
                f32 a = sq(mat->roughness);
                f32 f = fresnel_dielectric(wi, m, mat->ext_ior / mat->int_ior);
                f32 d = ggx_normal_distribution(a, no, m);
                f32 g = ggx_visibility_term(a, wi, wo, no, m);
                f32 j = 1.0 / (4.0 * mdoto);
                
                Vec3 specular = sample_texture(world, mat->specular, &hrec);
                Vec3 diffuse = sample_texture(world, mat->diffuse, &hrec);

                srec->bsdf = v3add(v3muls(diffuse, INV_PI * ndoto * (1.0 - f)), 
                                   v3muls(specular, f * d * g / (4.0 / ndoti)));
                srec->pdf = INV_PI * ndoto * (1.0 - f) 
                            + d * ndotm * j * f;
                srec->weight = v3divs(srec->bsdf, srec->pdf);
            }
        } break;
        case MaterialType_Dielectric: {
            Vec3 n = hrec.no;
            f32 eta = mat->int_ior / mat->ext_ior;
            if (hrec.is_front_face) {
                eta = 1.0f / eta;
            }
            f32 ndoti = -hrec.ndotio;
            f32 a = remap_roughness(mat->roughness, ndoti);
            f32 ndoto = dot(n, wo);
            bool is_reflection = ndoto * ndoti > 0.0;
            
            Vec3 m = normalize(is_reflection ? v3sub(wo, wi) : v3sub(v3muls(wi, eta), wo));
            if (dot(n, m) < 0.0) {
                m = v3neg(m);
            }
            
            f32 mdoti = -dot(m, wi);
            f32 mdoto = dot(m, wo);
            f32 ndotm = dot(n, m);
            
            f32 f = fresnel_dielectric(wi, m, eta);
            f32 d = ggx_normal_distribution(a, n, m);
            f32 g = ggx_visibility_term(a, wi, wo, n, m);
            
            if (is_reflection) {
                f32 j = 1.0 / (4.0 * mdoto);
                
                Vec3 specular = sample_texture(world, mat->specular, &hrec);
                srec->bsdf = v3muls(specular, d * g * f / (4.0 * ndoti));
                srec->pdf = f * j;
                srec->weight = specular;
            } else {
                f32 j = mdoti / sq(mdoti * eta + mdoto);
                f32 value = (1.0 - f) * d * g * mdoti * mdoto / (ndoti * sq(mdoti * eta + mdoto));
                Vec3 transmittance = sample_texture(world, mat->transmittance, &hrec);
                srec->bsdf = v3muls(transmittance, abs32(value));
                srec->pdf = (1.0 - f) * j;
                srec->weight = transmittance;
            }
            
            srec->pdf *= d * ndotm;
            srec->weight = v3muls(srec->weight, abs32(g * mdoti / (ndoti * ndotm)));
        } break;
        case MaterialType_DiffuseLight: {
            return false;
        } break;
        case MaterialType_Isotropic: {
            f32 ndoto = dot(no, wo);

            if (ndoto >= 0.0) {
                Vec3 atten = sample_texture(world, mat->diffuse, &hrec);
                srec->bsdf = v3muls(atten, 0.25f * INV_PI);
                srec->pdf = 0.25f * INV_PI;
                srec->weight = atten;
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    return true;
}

Vec3 
material_emit(World *world, Ray ray, HitRecord hrec, RayCastData data) {
    Vec3 result = {0};
    
    Material *mat = get_material(world, hrec.mat);
    switch(mat->type) {
        case MaterialType_DiffuseLight: {
            bool is_front_face = hrec.is_front_face;
            if (mat->light_flags & LightFlags_FlipFace) {
                is_front_face = !is_front_face;
            }
            
            if (!(mat->light_flags & LightFlags_BothSided) && !is_front_face) {
                break;
            }
            
            result = sample_texture(world, mat->emittance, &hrec);
        } break;
        default: {
            
        } break;
    }

    return result;
}

Bounds3 
get_object_bounds(World *world, ObjectHandle obj_handle) {
    Bounds3 result = bounds3empty();
    
    Object *obj = get_object(world, obj_handle);
    switch (obj->type) {
        case ObjectType_ObjectList: {
            if (obj->obj_list.size) {
                bool is_first_box = true;
                for (u64 obj_index = 0;
                     obj_index < obj->obj_list.size;
                     ++obj_index) {
                    ObjectHandle test_object = object_list_get(&obj->obj_list, obj_index);
                    Bounds3 temp_box = get_object_bounds(world, test_object);
                    result = is_first_box ? temp_box : bounds3_join(result, temp_box);
                    is_first_box = false;
                }
            }
        } break;
        case ObjectType_Sphere: {
            Vec3 rv = v3s(obj->sphere.r);
            result.min = v3sub(obj->sphere.p, rv);
            result.max = v3add(obj->sphere.p, rv);
        } break;
        case ObjectType_Triangle: {
            Vec3 epsilon = v3s(0.001f);
            result = bounds3empty();
            result = bounds3_extend(result, obj->triangle.p[0]);
            result = bounds3_extend(result, obj->triangle.p[1]);
            result = bounds3_extend(result, obj->triangle.p[2]);
            result.min = v3sub(result.min, epsilon);
            result.max = v3add(result.max, epsilon);
        } break;
        case ObjectType_ConstantMedium: {
            result = get_object_bounds(world, obj->constant_medium.boundary);
        } break;
        case ObjectType_Transform: {
            result = obj->transform.bounds;
        } break;
        case ObjectType_AnimatedTransform: {
            result = obj->animated_transform.bounds;
        } break;
        case ObjectType_BVH: {
            result = obj->bvh_node.bounds;
        } break;
        case ObjectType_Box: {
            result = obj->box.bounds;
        } break;
        case ObjectType_TriangleMesh: {
            result = obj->triangle_mesh.bounds;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

f32 
get_object_pdf_value(World *world, ObjectHandle obj_handle, Vec3 orig, Vec3 v,
                     RayCastData data){
    f32 result = 0;
    
    Object *obj = get_object(world, obj_handle);
    switch (obj->type) {
        case ObjectType_Triangle: {
            HitRecord hrec;
            Ray ray = make_ray(orig, v, 0);
            if (object_hit(world, ray, obj_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 surface_area = triangle_area(obj->triangle.p[0], obj->triangle.p[1], obj->triangle.p[2]);
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_Sphere: {
            HitRecord hrec;
            Ray ray = make_ray(orig, v, 0);
            if (object_hit(world, ray, obj_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 cos_theta_max = sqrt32(1 - obj->sphere.r * obj->sphere.r / length_sq(v3sub(obj->sphere.p, orig)));
                f32 solid_angle = TWO_PI * (1 - cos_theta_max);
                
                result = 1.0f / solid_angle;
            }
        } break;
        case ObjectType_Disk: {
            HitRecord hrec;
            Ray ray = make_ray(orig, v, 0);
            if (object_hit(world, ray, obj_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 surface_area = PI * obj->disk.r * obj->disk.r;
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_TriangleMesh: {
            HitRecord hrec;
            Ray ray = make_ray(orig, v, 0);
            if (object_hit(world, ray, obj_handle, 0.001f, INFINITY, &hrec, data)) {
                f32 surface_area = obj->triangle_mesh.surface_area;
                f32 distance_squared = hrec.t * hrec.t * length_sq(v);
                f32 cosine = abs32(dot(v, hrec.n) / length(v));
                result = distance_squared / (cosine * surface_area);
            }
        } break;
        case ObjectType_ObjectList: {
            f32 weight = 1.0f / obj->obj_list.size;
            f32 sum = 0;
            
            for (u32 obj_index = 0;
                 obj_index < obj->obj_list.size;
                 ++obj_index) {
                sum += weight * get_object_pdf_value(world, object_list_get(&obj->obj_list, obj_index), orig, v, data);        
            }
			result = sum;
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

Vec3 
get_object_random(World *world, ObjectHandle obj_handle, Vec3 o, RayCastData data) {
    Vec3 result = {0};
    Object *obj = get_object(world, obj_handle);
    switch (obj->type) {
        case ObjectType_Triangle: {
            f32 r1 = sqrt32(randomu(data.entropy));
            f32 r2 = randomu(data.entropy);
            result = v3add3(v3muls(obj->triangle.p[0], 1.0f - r1),
                            v3muls(obj->triangle.p[1], r1 * (1.0f - r2)),
                            v3muls(obj->triangle.p[2], r1 * r2));
            result = v3sub(result, o);
        } break;
        case ObjectType_Disk: {
            ONB uvw = onb_from_w(obj->disk.n);
            result = v3add(onb_local(uvw, v3muls(random_unit_disk(data.entropy), obj->disk.r)), obj->disk.p);
        } break;
        case ObjectType_Sphere: {
            Vec3 dir = v3sub(obj->sphere.p, o);
            f32 dist_sq = length_sq(dir);
            ONB uvw = onb_from_w(dir);
            result = onb_local(uvw, random_to_sphere(data.entropy, obj->sphere.r, dist_sq));
        } break;
        case ObjectType_TriangleMesh: {
            u32 tri_idx = random_int(data.entropy, obj->triangle_mesh.ntrig);
            Vec3 p[] = {
                obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[tri_idx * 3]],
                obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[tri_idx * 3 + 1]],
                obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[tri_idx * 3 + 2]],
            };
            
            f32 r1 = sqrt32(randomu(data.entropy));
            f32 r2 = randomu(data.entropy);
            result = v3add3(v3muls(p[0], 1.0f - r1),
                            v3muls(p[1], 1.0f - r2),
                            v3muls(p[2], r1 * r2));
            result = v3sub(result, o);
        } break;
        case ObjectType_ObjectList: {
            if (obj->obj_list.size) {
                u32 random_index = random_int(data.entropy, obj->obj_list.size);
                result = get_object_random(world, object_list_get(&obj->obj_list, random_index), o, data);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    return result;
}

bool 
object_hit(World *world, Ray ray, ObjectHandle obj_handle, f32 t_min, f32 t_max,
           HitRecord *hrec, RayCastData data) {
    bool result = false;
    
    ++data.stats->object_collision_tests;
    
    Object *obj = get_object(world, obj_handle);
    switch(obj->type) {
        case ObjectType_Sphere: {
            Vec3 rel_orig = v3sub(ray.orig, obj->sphere.p);
            f32 a = length_sq(ray.dir);
            f32 half_b = dot(rel_orig, ray.dir);
            f32 c = length_sq(rel_orig) - obj->sphere.r * obj->sphere.r;
            f32 discriminant = half_b * half_b - a * c;
            if (discriminant >= 0) {
                f32 root_term = sqrt32(discriminant);
                f32 tp = (-half_b + root_term) / a;
                f32 tn = (-half_b - root_term) / a;
                f32 t = tp;
                if ((tn > t_min) && (tn < tp)) {
                    t = tn;
                }
                if ((t > t_min) && (t < t_max)) {
                    hrec->t = t;
                    hrec->p = ray_at(ray, hrec->t);
                    Vec3 outward_normal = v3divs(v3sub(hrec->p, obj->sphere.p), obj->sphere.r);
                    hit_set_normal(hrec, outward_normal, ray);
                    f32 u, v;
                    sphere_get_uv(outward_normal, &u, &v);
                    hrec->u = u;
                    hrec->v = v;
                    
                    hrec->mat = obj->sphere.mat;
                    hrec->obj = obj_handle;
                    result = true;
                }
            }
        } break;
        case ObjectType_Disk: {
            f32 d = dot(obj->disk.n, ray.dir);
            if ((d < -0.001f) || (d > 0.001f)) {
                Vec3 ro = v3sub(obj->disk.p, ray.orig); 
                f32 t = dot(ro, obj->disk.n) / d;
                Vec3 hp = ray_at(ray, t);
                f32 dtcsq = length_sq(v3sub(hp, obj->disk.p));
                if ((t > t_min) && (t < t_max) && (dtcsq < obj->disk.r * obj->disk.r)) {
                    hrec->t = t;
                    hrec->p = hp;
                    hit_set_normal(hrec, obj->disk.n, ray);
                    hrec->mat = obj->disk.mat;
                    hrec->obj = obj_handle;
                    result = true;
                }
            }
        } break;
        case ObjectType_Triangle: {
            ++data.stats->ray_triangle_collision_tests;
           
           f32 t, u, v;
           if (triangle_hit(obj->triangle.p[0], obj->triangle.p[1], obj->triangle.p[2], ray, 
                &t, &u, &v, data.stats)) {
                if ((t > t_min) && (t < t_max)) {
                    hrec->t = t;
                    hrec->p = ray_at(ray, hrec->t);
                    // Vec3 outward_normal = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
                    Vec3 outward_normal = obj->triangle.n;
                    hit_set_normal(hrec, outward_normal, ray);
                    // @NOTE these are not actual uvs
                    hrec->u = u;
                    hrec->v = v;
                    
                    hrec->mat = obj->triangle.mat;
                    hrec->obj = obj_handle;
                    result = true;
                }
           }
        } break;
        case ObjectType_ObjectList: {
            bool has_hit_anything = false;
            
            f32 closest_so_far = t_max;
            for (u64 obj_index = 0;
                obj_index < obj->obj_list.size;
                ++obj_index) {
                ObjectHandle test_object = object_list_get(&obj->obj_list, obj_index);
              
                HitRecord temp_hit;  
                if (object_hit(world, ray, test_object, t_min, closest_so_far, &temp_hit, data)) {
                    has_hit_anything = true;
                    closest_so_far = temp_hit.t;
                    *hrec = temp_hit;
                }
            }
            
            result = has_hit_anything;
        } break;
        case ObjectType_ConstantMedium: {
            HitRecord hit1, hit2;
            if (object_hit(world, ray, obj->constant_medium.boundary, -INFINITY, INFINITY, &hit1, data)) {
                if (object_hit(world, ray, obj->constant_medium.boundary, hit1.t + 0.0001f, INFINITY, &hit2, data)) {
                    if (hit1.t < t_min) {
                        hit1.t = t_min;
                    }
                    if (hit2.t > t_max) {
                        hit2.t = t_max;
                    }
                    
                    if (hit1.t < hit2.t) {
                        if (hit1.t < 0) {
                            hit1.t = 0;
                        }
                        
                        f32 distance_inside_boundary = hit2.t - hit1.t;
                        f32 hit_dist = obj->constant_medium.neg_inv_density * logf(randomu(data.entropy));
                        
                        if (hit_dist < distance_inside_boundary) {
                            hrec->t = hit1.t + hit_dist;
                            hrec->p = ray_at(ray, hrec->t);
                            
                            // @NOTE can be not set bacuse not used
                            // hrec->n = v3(1, 0, 0);
                            // hrec->is_front_face = true;
                            hrec->mat = obj->constant_medium.phase_function;
                            hrec->obj = obj_handle;;
                            
                            result = true;
                        } 
                    }
                }
            }
        } break;
        case ObjectType_Transform: {
            // @TODO something is wrong with hitting instances or bvhs
            Vec3 os_orig = mat4x4_mul_vec3(obj->transform.t.w2o, ray.orig);
            Vec3 os_dir = mat4x4_as_3x3_mul_vec3(obj->transform.t.w2o, ray.dir); 
            Ray os_ray = make_ray(os_orig, os_dir, ray.time);
        
            result = object_hit(world, os_ray, obj->transform.obj, t_min, t_max, hrec, data);
            if (result) {
                Vec3 ws_p = mat4x4_mul_vec3(obj->transform.t.o2w, hrec->p);
                Vec3 ws_n = normalize(mat4x4_as_3x3_mul_vec3(obj->transform.t.o2w, hrec->n));
                
                hrec->obj = obj_handle;
                hrec->p = ws_p;
                hit_set_normal(hrec, ws_n, ray);   
            }
        } break;
        case ObjectType_AnimatedTransform: {
            f32 ray_time = ray.time;
            f32 time = (ray_time - obj->animated_transform.time[0]) / (obj->animated_transform.time[1] - obj->animated_transform.time[0]);
            Vec3 t = v3lerp(obj->animated_transform.t[0], obj->animated_transform.t[1], time);
            Quat4 r = q4lerp(obj->animated_transform.r[0], obj->animated_transform.r[1], time);
            Transform trans = transform_tr(t, r);
            
            Vec3 os_orig = mat4x4_mul_vec3(trans.w2o, ray.orig);
            Vec3 os_dir = mat4x4_as_3x3_mul_vec3(trans.w2o, ray.dir); 
            Ray os_ray = make_ray(os_orig, os_dir, ray.time);
        
            result = object_hit(world, os_ray, obj->animated_transform.obj, t_min, t_max, hrec, data);
            if (result) {
                Vec3 ws_p = mat4x4_mul_vec3(trans.o2w, hrec->p);
                Vec3 ws_n = normalize(mat4x4_as_3x3_mul_vec3(trans.o2w, hrec->n));
                
                hrec->obj = obj_handle;
                hrec->p = ws_p;
                hit_set_normal(hrec, ws_n, ray);   
            }
        } break;
        case ObjectType_BVH: {
            if (bounds3_hit(obj->bvh_node.bounds, ray, t_min, t_max)) {
                bool hit_left = object_hit(world, ray, obj->bvh_node.left, t_min, t_max, hrec, data);
                bool hit_right = object_hit(world, ray, obj->bvh_node.right, t_min, hit_left ? hrec->t : t_max, hrec, data);
                result = hit_left || hit_right;
            }
        } break;
        case ObjectType_Box: {
            result = object_hit(world, ray, obj->box.sides, t_min, t_max, hrec, data);
            hrec->obj = obj_handle;
        } break;
        case ObjectType_TriangleMesh: {
            if (!bounds3_hit(obj->triangle_mesh.bounds, ray, t_min, t_max)) {
                break;
            }
            
            f32 hit_u, hit_v, hit_t;
            u32 hit_vertex_index = 0;
            
            for (u32 triangle_index = 0;
                 triangle_index < obj->triangle_mesh.ntrig;
                 ++triangle_index) {
                u32 vertex_index = triangle_index * 3;
                Vec3 p0 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index]];        
                Vec3 p1 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index + 1]];        
                Vec3 p2 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[vertex_index + 2]];        
                f32 t, u, v;
                if (triangle_hit(p0, p1, p2, ray, &t, &u, &v, data.stats)) {
                    if ((t > t_min) && (t < t_max)) {
                        t_max = t;
                        
                        hit_u = u;
                        hit_v = v;
                        hit_t = t;
                        
                        hit_vertex_index = vertex_index;                        
                        result = true;
                    }
                }
            }
            
            if (result) {
                hrec->t = hit_t;
                hrec->p = ray_at(ray, hrec->t);
                
#if 1
                Vec3 n0 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index]];        
                Vec3 n1 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index + 1]];    
                Vec3 n2 = obj->triangle_mesh.n[obj->triangle_mesh.tri_indices[hit_vertex_index + 2]];    
                Vec3 outward_normal = normalize(v3add3(v3muls(n0, 1 - hit_u - hit_v),
                                                       v3muls(n1, hit_u),
                                                       v3muls(n2, hit_v)));
#else 
                Vec3 p0 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[hit_vertex_index]];        
                Vec3 p1 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[hit_vertex_index + 1]];        
                Vec3 p2 = obj->triangle_mesh.p[obj->triangle_mesh.tri_indices[hit_vertex_index + 2]];    
                Vec3 outward_normal = normalize(cross(v3sub(p1, p0), v3sub(p2, p0)));
#endif 
                // hrec->n = outward_normal;
                // hrec->is_front_face = true;
                hit_set_normal(hrec, outward_normal, ray);
                Vec2 uv0 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index]];        
                Vec2 uv1 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index + 1]];    
                Vec2 uv2 = obj->triangle_mesh.uv[obj->triangle_mesh.tri_indices[hit_vertex_index + 2]];    
                Vec2 uv = v2add3(v2muls(uv0, 1 - hit_u - hit_v), v2muls(uv1, hit_u), v2muls(uv2, hit_v));
                hrec->u = uv.x;
                hrec->v = uv.y;
                
                hrec->mat = obj->triangle_mesh.mat;
                hrec->obj = obj_handle;
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
    
    data.stats->object_collision_test_successes += result;
    
    return result;
}

Vec3 
ray_cast(World *world, Ray ray, i32 depth, RayCastData data) {
    Vec3 radiance = v3s(0);
    Vec3 throughput = v3s(1.0);
    
    for(u32 bounce = 0;
        bounce < depth;
        ++bounce) {
        ++data.stats->bounce_count;
        
        HitRecord hrec = {0};
        if (!object_hit(world, ray, world->obj_list, DISTANCE_EPSILON, INFINITY, &hrec, data)) {
            radiance = v3add(radiance, v3mul(throughput, world->backgorund_color));
            break;
        }    
        
        Vec3 emitted = material_emit(world, ray, hrec, data);
        if (length_sq(emitted) > 0) {
            radiance = v3add(radiance, v3mul(throughput, emitted));
        }
        
        ScatterRecord srec = {0};
        if (!material_scatter(world, ray, hrec, data, &srec)) {
            break;
        }
        if (!material_compute_scattering_functions(world, ray.dir, hrec.n, hrec, &srec, data)) {
            break;
        }
        
        // if (get_material(world, hrec.mat)->type == MaterialType_Lambertian) {
            // Vec3 dir;
            // if (randomu(data.entropy) < 0.5) {
            //     dir = normalize(get_object_random(world, world->important_objects, hrec.p, data));    
            // } else {
            //     dir = srec.dir;
            // }
            
            // srec.dir = dir;    
            // f32 light_pdf = get_object_pdf_value(world, world->important_objects, hrec.p, dir, data);
            // f32 pdf = 0.5f * srec.pdf + 0.5f * light_pdf;
            // srec.weight = v3divs(srec.bsdf, pdf);
            
        //     Vec3 dir = normalize(get_object_random(world, world->important_objects, hrec.p, data));    
        //     f32 light_pdf = get_object_pdf_value(world, world->important_objects, hrec.p, dir, data);
        //     srec.weight = v3divs(srec.bsdf, light_pdf);
        // }
        
        // if (randomu(data.entropy) < 0.5) {
        //     srec.dir = normalize(get_object_random(world, world->important_objects, hrec.p, data));    
        // }
        // f32 light_pdf = get_object_pdf_value(world, world->important_objects, hrec.p, srec.dir, data);
        // f32 pdf = 0.5f * light_pdf + srec.pdf * 0.5f;
        // srec.weight = v3divs(srec.bsdf, pdf);
        
        // srec.weight = v3divs(srec.bsdf, srec.pdf);
       
        
        if (is_black(srec.weight)) {
            break;
        }
        
        throughput = v3mul(throughput, srec.weight);
        ray = make_ray(hrec.p, srec.dir, ray.time);
        
#if ENABLE_RUSSIAN_ROULETTE
        if (bounce > 3) {
            f32 p = max32(max32(throughput.x, throughput.y), throughput.z);
            if (randomu(data.entropy) > min32(p, 0.95f)) {
                ++data.stats->russian_roulette_terminated_bounces;
                break;
            }
            throughput = v3muls(throughput, 1.0f / p);
        }
#endif 
    }
    
    return radiance;
}
