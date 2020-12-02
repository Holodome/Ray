
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

    MaterialHandle sphere_mat = add_material(world, (Material) {
        .type = MaterialType_Metal,
        .metal = {
            .albedo = add_texture(world, texture_solid(v3(0.7, 0.3, 0.1))),
            .fuzz = 0.1
        }
    });
    add_object_to_list(world, world->object_list, add_object(world, object_sphere(v3(350, 400, 600), 75, sphere_mat)));

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
