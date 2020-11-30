#include "general.h"
#include "image.h"
#include "trace.h"
#include "perlin.h"

typedef struct {
    u32 x_min;
    u32 x_max;
    u32 y_min;
    u32 y_max;
    
    RandomSeries entropy;
} RenderWorkOrder;

typedef struct {
    Image *output;
    
    World *world;
    
    RenderWorkOrder *orders;
    u32 order_count;
    
    volatile u64 next_order_index;
    volatile u64 orders_done;
    
    volatile u64 total_bounce_count;
    volatile u64 total_ray_triangle_collision_tests;
} RenderWorkQueue;

typedef struct {
    u64 id;
} Thread;

#define THREAD_PROC_SIGNATURE(_name) unsigned long _name(void *param)
typedef THREAD_PROC_SIGNATURE(ThreadProc);

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

static u64
atomic_add64(volatile u64 *value, u64 addend) {
	u64 result = InterlockedExchangeAdd64((volatile long long *)value, addend);
	return result;
}

static Thread
create_thread(ThreadProc *proc, void *param) {
	Thread result = {0};
	
	DWORD thread_id;
	HANDLE thread_handle = CreateThread(0, 0, proc, param, 0, &thread_id);
	CloseHandle(thread_handle);
	
	result.id = thread_id;
	
	return result;
}

static void
exit_thread(void) {
    ExitThread(0);
}

static bool 
render_tile(RenderWorkQueue *queue) {
    u64 work_order_index = atomic_add64(&queue->next_order_index, 1);
    if (work_order_index >= queue->order_count) {
        return false;
    }
    
    RenderWorkOrder *order = queue->orders + work_order_index;
    
    u64 bounce_count = 0;
    u64 ray_triangle_collision_tests = 0;
    for (u32 y = order->y_min;
         y < order->y_max;
         ++y) {
        u32 *pixel = image_get_pixel_pointer(queue->output, order->x_min, y);
             
        for (u32 x = order->x_min;
             x < order->x_max;
             ++x) {
            Vec3 pixel_color = v3(0, 0, 0);
            
            for (u32 sample_index = 0;
                sample_index < SAMPLES_PER_PIXEL;
                ++sample_index) {
                f32 u = ((f32)x + random_bilateral(&order->entropy) * 0.5f) / (f32)queue->output->w;
                f32 v = ((f32)y + random_bilateral(&order->entropy) * 0.5f) / (f32)queue->output->h;
                Ray ray = camera_make_ray(&queue->world->camera, &order->entropy, u, v);
                
                RayCastData cast_data = {0};
                Vec3 sample_color = ray_cast(queue->world, &order->entropy, ray, &cast_data);
                bounce_count += cast_data.bounce_count;
                ray_triangle_collision_tests += cast_data.ray_triangle_collision_tests;
                
                f32 color_multiplier = 1.0f / (f32)SAMPLES_PER_PIXEL;
                pixel_color = v3add(pixel_color, v3muls(sample_color, color_multiplier));
            }

            *pixel++ = rgba_pack_4x8_linear1(sqrtf(pixel_color.b), sqrtf(pixel_color.g), sqrtf(pixel_color.r), 1.0f);     
        }        
    }
    
    atomic_add64(&queue->orders_done, 1);
    atomic_add64(&queue->total_bounce_count, bounce_count);
    atomic_add64(&queue->total_ray_triangle_collision_tests, ray_triangle_collision_tests);
    
    return true;
}

static THREAD_PROC_SIGNATURE(render_thread_proc) {
    RenderWorkQueue *queue = param;
    
    while (render_tile(queue)) { }
    
    exit_thread();
    return 0;
}

#if 0
static void 
init_scene2(World *world, Image *image) {
    MaterialHandle ground_mat = add_material(world, (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = add_texture(world, (Texture) {
                .type = TextureType_Checkered,
                .checkered = {
                    .t1 = add_texture(world, (Texture) {
                        .type = TextureType_Solid,
                        .solid = {
                            .c = v3(0.2, 0.3, 0.1)
                        }
                    }),
                    .t2 = add_texture(world, (Texture) {
                        .type = TextureType_Solid,
                        .solid = {
                            .c = v3(0.9, 0.9, 0.9)
                        }
                    })
                }
            })
        }
    });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .mat = ground_mat,
            .p = v3(0, -1000, 0),
            .r = 1000.0f
        }
    });
    
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            f32 choose_mat = random(&global_entropy);
            Vec3 center = v3(a + 0.9f * random(&global_entropy),
                             0.2,
                             b + 0.9 * random(&global_entropy));

            if (length(v3sub(center, v3(4, 0.2, 0))) > 0.9) {
                if (choose_mat < 0.8) {
                    // diffuse
                    Vec3 albedo = v3mul(random_vector(&global_entropy, 0.1f, 1.0f), random_vector(&global_entropy, 0.1f, 1.0f));
                    
                    TextureHandle tex = add_texture(world, (Texture) {
                        .type = TextureType_Solid,
                        .solid = {
                            .c = albedo
                        }
                    });
                    MaterialHandle mat = add_material(world, (Material) {
                        .type = MaterialType_Lambertian,
                        .lambertian = {
                            .albedo = tex
                        }
                    });
                    add_object(world, (Object) {
                        .type = ObjectType_Sphere,  
                        .sphere = {
                            .mat = mat,
                            .p = center,
                            .r = 0.2f
                        }
                    });
                } else if (choose_mat < 0.95) {
                    // metal
                    f32 fuzz = random_uniform(&global_entropy, 0, 0.5);
                    Vec3 albedo = random_vector(&global_entropy, 0.5, 1);
                    TextureHandle tex = add_texture(world, (Texture) {
                        .type = TextureType_Solid,
                        .solid = {
                            .c = albedo
                        }
                    });
                    MaterialHandle mat = add_material(world, (Material) {
                        .type = MaterialType_Metal,
                        .metal = {
                            .albedo = tex,
                            .fuzz = fuzz
                        }
                    });
                    add_object(world, (Object) {
                        .type = ObjectType_Sphere,  
                        .sphere = {
                            .mat = mat,
                            .p = center,
                            .r = 0.2f
                        }
                    });
                } else {
                    // glass
                    MaterialHandle mat = add_material(world, (Material) {
                        .type = MaterialType_Dielectric,
                        .dielectric = {
                            .ir = 1.5f
                        }
                    });
                    add_object(world, (Object) {
                        .type = ObjectType_Sphere,  
                        .sphere = {
                            .mat = mat,
                            .p = center,
                            .r = 0.2f
                        }
                    });
                }
            }
        }
    }
    
    MaterialHandle mat1 = add_material(world, (Material) {
        .type = MaterialType_Dielectric,
        .dielectric = {
            .ir = 1.5f
        }
    });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .mat = mat1,
            .p = v3(0, 1, 0),
            .r = 1.0f
        }
    });
    
    MaterialHandle mat2 = add_material(world, (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = add_texture(world, (Texture) {
                .type = TextureType_Solid,
                .solid = {
                    .c = v3(0.4, 0.2, 0.1)
                }
            })
        }
    });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .mat = mat2,
            .p = v3(-4, 1, 0),
            .r = 1.0f
        }
    });
    
    MaterialHandle mat3 = add_material(world, (Material) {
        .type = MaterialType_Metal,
        .metal = {
            .fuzz = 0,
            .albedo = add_texture(world, (Texture) {
                .type = TextureType_Solid,
                .solid = {
                    .c = v3(0.7, 0.6, 0.5)
                }
            })
        }
    });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,  
        .sphere = {
            .mat = mat3,
            .p = v3(4, 1, 0),
            .r = 1.0f
        }
    });
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;

    world->backgorund_color = v3(0.70, 0.80, 1.00);

    Vec3 look_from = v3(13, 2, 3);
    Vec3 look_at = v3(0, 0, 0);
    f32 dtf = 10.0f;
    f32 aperture = 0.1f;
    world->camera = make_camera(look_from, look_at, v3(0, 1, 0), aspect_ratio, DEG2RAD(20), aperture, dtf);
}

void 
init_scene1(World *world, Image *image) {
    MaterialHandle marble = add_material(world, (Material) {
            .type = MaterialType_Lambertian,
            .lambertian = {
                .albedo = add_texture(world, (Texture) {
                    .type = TextureType_Perlin,
                    .perlin = {
                        .p = make_perlin(&global_entropy),
                        .s = 4.0f
                    }
                })
            }
        });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,
        .sphere = {
            .mat = marble,
            .p = v3(0, 2, 0),
            .r = 2
        },
        // .mat = add_material(world, (Material) {
        //     .type = MaterialType_Lambertian,
        //     .lambertian = {
        //         .albedo = add_texture(world, (Texture) {
        //             .type = TextureType_Normal
        //         })
        //     } 
        // })
    });
    add_object(world, (Object) {
        .type = ObjectType_Sphere,
        .sphere = {
            .p = v3(0, -1000, 0),
            .r = 1000,
            .mat = marble
        },
    });
    
    MaterialHandle light = add_material(world, (Material) {
        .type = MaterialType_DiffuseLight,
        .diffuse_light = {
            .emit = add_texture(world, (Texture) {
                .type = TextureType_Solid,
                .solid = {
                    .c = v3s(1)
                }
            })
        } 
    });
    add_object(world, (Object) {
        .type = ObjectType_Triangle,
        .triangle = {
            .mat = light,
            .p[0] = v3(3, 1, -2),
            .p[1] = v3(5, 1, -2),
            .p[2] = v3(5, 3, -2),
        } 
    });
    add_object(world, (Object) {
        .type = ObjectType_Triangle,
        .triangle = {
            .mat = light,
            .p[0] = v3(3, 1, -2),
            .p[1] = v3(3, 3, -2),
            .p[2] = v3(5, 3, -2),
        } 
    });
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;

    world->backgorund_color = v3(0, 0, 0);

    Vec3 look_from = v3(26, 3, 6);
    Vec3 look_at = v3(0, 2, 0);
    f32 dtf = length(v3sub(look_from, look_at));
    f32 aperture = 0.0f;
    world->camera = make_camera(look_from, look_at, v3(0, 1, 0), aspect_ratio, DEG2RAD(20), aperture, dtf);
}
#endif 

void 
init_cornell_box(World *world, Image *image) {
    MaterialHandle red = add_material(world, material_lambertian(add_texture(world, texture_solid(v3(0.65, 0.05, 0.05)))));
    MaterialHandle white = add_material(world, material_lambertian(add_texture(world, texture_solid(v3(0.73, 0.73, 0.73)))));
    MaterialHandle green = add_material(world, material_lambertian(add_texture(world, texture_solid(v3(0.12, 0.45, 0.15)))));
    MaterialHandle light = add_material(world, (Material) {
        .type = MaterialType_DiffuseLight,
        .diffuse_light = {
            .emit = add_texture(world, texture_solid(v3s(7)))
        }
    });
    
    world->object_list = add_object(world, (Object) {
        .type = ObjectType_ObjectList
    });
    add_yz_rect(world, world->object_list, 0, 555, 0, 555, 555, green);
    add_yz_rect(world, world->object_list, 0, 555, 0, 555, 0, red);
    // add_xz_rect(world, world->object_list, 213, 343, 227, 332, 554, light);
    add_xz_rect(world, world->object_list, 113, 443, 127, 432, 554, light);
    add_xz_rect(world, world->object_list, 0, 555, 0, 555, 0, white);
    add_xz_rect(world, world->object_list, 0, 555, 0, 555, 555, white);
    add_xy_rect(world, world->object_list, 0, 555, 0, 555, 555, white);
    
    ObjectHandle box1_list = add_object(world, (Object) { .type = ObjectType_ObjectList });
    add_box(world, box1_list, v3(0, 0, 0), v3(165, 330, 165), white);
    ObjectHandle box1 = add_object(world, object_instance(box1_list, v3(265,0,295), v3(0, DEG2RAD(15), 0)));
    ObjectHandle smoke1 = add_object(world, (Object) {
        .type = ObjectType_ConstantMedium,
        .constant_medium = {
            .neg_inv_density = -1 / 0.01f,
            .phase_function = add_material(world, material_isotropic(add_texture(world, texture_solid(v3s(0))))),
            .boundary = box1
        }
    });
    
    ObjectHandle box2_list = add_object(world, (Object) { .type = ObjectType_ObjectList });
    add_box(world, box2_list, v3(0, 0, 0), v3(165, 165, 165), white);
    ObjectHandle box2 = add_object(world, object_instance(box2_list, v3(130, 0, 65), v3(0, DEG2RAD(-18), 0)));
    ObjectHandle smoke2 = add_object(world, (Object) {
        .type = ObjectType_ConstantMedium,
        .constant_medium = {
            .neg_inv_density = -1 / 0.01f,
            .phase_function = add_material(world, material_isotropic(add_texture(world, texture_solid(v3s(1))))),
            .boundary = box2
        }
    });
    
    ObjectHandle smoke_list = add_object(world, OBJECT_LIST);
    add_object_to_list(world, smoke_list, smoke1);
    add_object_to_list(world, smoke_list, smoke2);
    add_object_to_list(world, world->object_list, add_object(world, object_bvh_node(world, world->objects[smoke_list.v].object_list.objects, 2, 0, 2)));
    
    world->backgorund_color = v3s(0);

    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(278, 278, -800);
    Vec3 look_at = v3(278, 278, 0);
    f32 dtf = 10;
    f32 aperture = 0.f;
    world->camera = make_camera(look_from, look_at, v3(0, 1, 0), aspect_ratio, DEG2RAD(40), aperture, dtf);
}

void 
init_scene3(World *world, Image *image) {
    world->object_list = add_object(world, OBJECT_LIST);
    MaterialHandle ground = add_material(world, material_lambertian(add_texture(world, texture_solid(v3(0.48, 0.83, 0.53)))));

    ObjectHandle boxes_list = add_object(world, OBJECT_LIST);
    const int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            f32 w = 100.0;
            f32 x0 = -1000.0 + i*w;
            f32 z0 = -1000.0 + j*w;
            f32 y0 = 0.0;
            f32 x1 = x0 + w;
            f32 y1 = random_uniform(&global_entropy, 1,101);
            f32 z1 = z0 + w;

            add_object_to_list(world, boxes_list, add_object(world, object_box(world, v3(x0,y0,z0), v3(x1,y1,z1), ground)));
        }
    }
    add_object_to_list(world, world->object_list, 
                       add_object(world,
                                  object_bvh_node(world, world->objects[boxes_list.v].object_list.objects, 
                                                  boxes_per_side * boxes_per_side, 0, boxes_per_side * boxes_per_side)));

    MaterialHandle light = add_material(world, (Material) {
        .type = MaterialType_DiffuseLight,
        .diffuse_light = {
            .emit = add_texture(world, texture_solid(v3s(7)))
        }
    });
    add_xz_rect(world, world->object_list, 123, 423, 147, 412, 554, light);

    add_object_to_list(world, world->object_list, add_object(world, object_sphere(v3(260, 150, 45), 50, add_material(world, material_dielectric(1.5)))));
    add_object_to_list(world, world->object_list, add_object(world, object_sphere(v3(0, 150, 145), 50, add_material(world, (Material) {
        .type = MaterialType_Metal,
        .metal = {
            .albedo = add_texture(world, texture_solid(v3(0.8, 0.8, 0.9))),
            .fuzz = 1
        }
    }))));

    ObjectHandle bound1 = add_object(world, object_sphere(v3(360,150,145), 70, add_material(world, material_dielectric(1.5f))));
    add_object_to_list(world, world->object_list, bound1);
    add_object_to_list(world, world->object_list, add_object(world, (Object) {
        .type = ObjectType_ConstantMedium,
        .constant_medium = {
            .neg_inv_density = -1 / 0.2f,
            .phase_function = add_material(world, material_isotropic(add_texture(world, texture_solid(v3(0.2, 0.4, 0.9))))),
            .boundary = bound1
        }
    }));
    
    ObjectHandle bound2 = add_object(world, object_sphere(v3(0,0,0), 5000, add_material(world, material_dielectric(1.5f))));
    add_object_to_list(world, world->object_list, bound2);
    add_object_to_list(world, world->object_list, add_object(world, (Object) {
        .type = ObjectType_ConstantMedium,
        .constant_medium = {
            .neg_inv_density = -1 / 0.0001f,
            .phase_function = add_material(world, material_isotropic(add_texture(world, texture_solid(v3s(1))))),
            .boundary = bound2
        }
    }));

    MaterialHandle emat = add_material(world, (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = add_texture(world, (Texture) {
                .type = TextureType_Image,
                .image = {
                    .i = load_bmp("earth.bmp")
                }
            })
        } 
    });
    add_object_to_list(world, world->object_list, add_object(world, object_sphere(v3(400,200,400), 100, emat)));
    MaterialHandle marble = add_material(world, (Material) {
        .type = MaterialType_Lambertian,
        .lambertian = {
            .albedo = add_texture(world, (Texture) {
                .type = TextureType_Perlin,
                .perlin = {
                    .p = make_perlin(&global_entropy),
                    .s = 0.1f
                }
            })
        }
    });  
    add_object_to_list(world, world->object_list, add_object(world, object_sphere(v3(220,280,300), 80, marble)));

    ObjectHandle sphere_box = add_object(world, OBJECT_LIST);
    MaterialHandle white = add_material(world, material_lambertian(add_texture(world, texture_solid(v3(.73, .73, .73)))));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        add_object_to_list(world, sphere_box, add_object(world, object_sphere(v3add(v3(-100,270,395), random_vector(&global_entropy, 0, 165)), 10, white)));
    }
    add_object_to_list(world, world->object_list, add_object(world, object_bvh_node(world, world->objects[sphere_box.v].object_list.objects, ns, 0, ns)));

    world->backgorund_color = v3s(0);

    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(478, 278, -600);
    Vec3 look_at = v3(278, 278, 0);
    f32 dtf = 10;
    f32 aperture = 0.f;
    world->camera = make_camera(look_from, look_at, v3(0, 1, 0), aspect_ratio, DEG2RAD(40), aperture, dtf);
}

int 
main(int argc, char **argv) {
    u32 image_w = 480;
    u32 image_h = 480;
    
    Image output_image = make_image_for_writing(image_w, image_h);
    
    World world = {0};
    init_scene3(&world, &output_image);
    
    u32 tile_w = 64;
    u32 tile_h = 64;
    u32 tile_count_x = (image_w + tile_w - 1) / tile_w;
    u32 tile_count_y = (image_h + tile_h - 1) / tile_h;
    u32 tile_count = tile_count_x * tile_count_y;
    
    RenderWorkQueue render_queue = {0};
    render_queue.output = &output_image;
    render_queue.world = &world;
    render_queue.order_count = tile_count;
    render_queue.orders = malloc(sizeof(RenderWorkOrder) * render_queue.order_count);
    
    u32 cursor = 0;
    for (u32 tile_y = 0;
         tile_y < tile_count_y;
         ++tile_y) {
        u32 y_min = tile_y * tile_h;
        u32 y_max = y_min + tile_h;
        if (y_max > image_h) {
            y_max = image_h;
        }
        
        for (u32 tile_x = 0;
             tile_x < tile_count_x;
             ++tile_x) {
            u32 x_min = tile_x * tile_w;
            u32 x_max = x_min + tile_w;
            if (x_max > image_w) {
                x_max = image_w;
            }
            
            RenderWorkOrder *order = render_queue.orders + cursor++;
            assert(cursor <= render_queue.order_count);
            
            order->x_min = x_min;
            order->x_max = x_max;
            order->y_min = y_min;
            order->y_max = y_max;
            
            u32 seed = tile_count_x * 13998 + tile_count_y * 39224 + tile_x * 60918 + tile_y * 14319;
            order->entropy.state = seed;
        }           
    }
    assert(cursor == render_queue.order_count);
    
    printf("Scene object count: %llu\n", world.objects_size);
    printf("Scene texture count: %llu\n", world.textures_size);
    printf("Scene material count: %llu\n", world.materials_size);
    printf("Scene memory taken: %llu bytes\n", world.objects_size * sizeof(*world.objects) + 
                                         world.textures_size * sizeof(*world.textures) +
                                         world.materials_size * sizeof(*world.materials));
    
    printf("Start raycasting\n");
    clock_t start_clock = clock();
    
    u32 thread_count = 6;
    for (u32 core_index = 1;
         core_index < thread_count;
         ++core_index) {
        create_thread(render_thread_proc, &render_queue);
    }
    
    while (render_queue.orders_done < render_queue.order_count) {
        if (render_tile(&render_queue)) {
            f32 percent = (f32)render_queue.orders_done / (f32)render_queue.order_count;
            printf("\rRaycasting %u%%", (u32)roundf(percent * 100));
            fflush(stdout);
        }
    }
    printf("\nRaycasting done\n");
    
    clock_t end_clock = clock();
    clock_t time_elapsed = end_clock - start_clock;
    
    printf("Raycasting time: %lldms\n", (i64)time_elapsed);
    printf("Pixel count: %u\n", output_image.w * output_image.h);
    printf("Samples per pixel: %llu\n", SAMPLES_PER_PIXEL);
    printf("Primary ray count: %llu\n", SAMPLES_PER_PIXEL * output_image.w * output_image.h);
    printf("Perfomace: %fms/primary ray\n", (f64)time_elapsed / (f64)(SAMPLES_PER_PIXEL * output_image.w * output_image.h));
    printf("Total bounces: %llu\n", render_queue.total_bounce_count);
    printf("Perfomance: %fms/bounce\n", (f64)time_elapsed / (f64)render_queue.total_bounce_count);
    printf("Triangle collision tests: %llu\n", render_queue.total_ray_triangle_collision_tests);
    printf("Object collision tests: %llu\n", render_queue.total_bounce_count * world.objects_size);
    printf("Average bounce count per ray: %f\n", (f64)render_queue.total_bounce_count / (f64)(SAMPLES_PER_PIXEL * output_image.w * output_image.h));
    
    char *out = "out.bmp";
    image_save(&output_image, out);
    char command[32] = {0};
    snprintf(command, sizeof(command), "start %s", out);
    system(command);
    
    printf("Exited successfully\n");
    return 0;
}
