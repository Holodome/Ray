
void 
init_scene2(World *world, Image *image) {
    world->backgorund_color = v3(0.70, 0.80, 1.00);  
    MaterialHandle ground_mat = material_lambertian(world,
        texture_checkerboard3d(world,
        texture_solid(world, v3(0.2, 0.3, 0.1)),
        texture_solid(world, v3(0.9, 0.9, 0.9))));
    add_object_to_world(world, object_sphere(world, v3(0, -1000, 0), 1000, ground_mat));
    
    for (i32 a = -11; 
         a < 11; 
         a++) {
        for (i32 b = -11;
             b < 11;
             b++) {
            f32 choose_mat = random(&global_entropy);
            Vec3 center = v3(a + 0.9f * random(&global_entropy),
                             0.2,
                             b + 0.9 * random(&global_entropy));

            if (length(v3sub(center, v3(4, 0.2, 0))) > 0.9) {
                if (choose_mat < 0.8) {
                    Vec3 albedo = v3mul(random_vector(&global_entropy, 0.1f, 1.0f), random_vector(&global_entropy, 0.1f, 1.0f));
                    MaterialHandle mat = material_lambertian(world, texture_solid(world, albedo));       
                    add_object_to_world(world, object_sphere(world, center, 0.2f, mat));
                } else if (choose_mat < 0.95) {
                    f32 fuzz = random_uniform(&global_entropy, 0, 0.5);
                    Vec3 albedo = random_vector(&global_entropy, 0.5, 1);
                    MaterialHandle mat = material_metal(world, texture_solid(world, albedo), fuzz);       
                    add_object_to_world(world, object_sphere(world, center, 0.2f, mat));
                } else {
                    MaterialHandle mat = material_dielectric(world, 1.5f);       
                    add_object_to_world(world, object_sphere(world, center, 0.2f, mat));
                }
            }
        }
    }
    
    MaterialHandle di_m = material_dielectric(world, 1.5f);       
    add_object_to_world(world, object_sphere(world, v3(0, 1, 0), 1, di_m));
    
    MaterialHandle la_m = material_lambertian(world, texture_solid(world, v3(0.4, 0.2, 0.1)));
    add_object_to_world(world, object_sphere(world, v3(-4, 1, 0), 1, la_m));
    
    MaterialHandle me_m = material_metal(world, texture_solid(world, v3(0.7, 0.6, 0.5)), 0);
    add_object_to_world(world, object_sphere(world, v3(4, 1, 0), 1, me_m));
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;

    Vec3 look_from = v3(13, 2, 3);
    // Vec3 look_from = v3(2, 2, 3);
    Vec3 look_at = v3(0, 0, 0);
    f32 dtf = 10.0f;
    f32 aperture = 0.1f;
    // world->camera = camera_environment(look_from, look_at, v3(0, 1, 0), 0, 1);
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(20), aperture, dtf, 0, 1);
}

void 
init_scene1(World *world, Image *image) {
    MaterialHandle marble = material_lambertian(world, texture_perlin(world, make_perlin(&world->arena, &global_entropy), 4.0f));
    add_object_to_world(world, object_sphere(world, v3(0, 2, 0), 2, marble));
    add_object_to_world(world, object_sphere(world, v3(0, -1000, 0), 1000, marble));
    
    MaterialHandle light = material_diffuse_light(world, texture_solid(world, v3s(2)), false);
    ObjectHandle lights = object_list(world);
    add_xy_rect(world, lights, 3, 5, 1, 3, -2, light);
    add_object_to_world(world, lights);
    add_important_object(world, lights);
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    
    world->backgorund_color = v3(0, 0, 0);

    Vec3 look_from = v3(26, 3, 10);
    Vec3 look_at = v3(0, 2, 0);
    f32 dtf = length(v3sub(look_from, look_at));
    f32 aperture = 0.0f;
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(20), aperture, dtf, 0, 1);
}

void 
init_cornell_box(World *world, Image *image) {
    MaterialHandle red   = material_lambertian(world, texture_solid(world, v3(0.65, 0.05, 0.05)));
    MaterialHandle white = material_lambertian(world, texture_solid(world, v3(0.73, 0.73, 0.73)));
    MaterialHandle green = material_lambertian(world, texture_solid(world, v3(0.12, 0.45, 0.15)));
    MaterialHandle light = material_diffuse_light(world, texture_solid(world, v3s(15)), false);
    
    add_yz_rect(world, world->obj_list, 0, 555, 0, 555, 555, green);
    add_yz_rect(world, world->obj_list, 0, 555, 0, 555, 0, red);
    ObjectHandle flipped = object_list(world);
    add_xz_rect(world, flipped, 213, 343, 227, 332, 554, light);
    add_object_to_world(world, flipped);
    // add_xz_rect(world, world->obj_list, 113, 443, 127, 432, 554, light);
    add_xz_rect(world, world->obj_list, 0, 555, 0, 555, 0, white);
    add_xz_rect(world, world->obj_list, 0, 555, 0, 555, 555, white);
    add_xy_rect(world, world->obj_list, 0, 555, 0, 555, 555, white);
    
    ObjectHandle light_list = object_list(world);
    add_xz_rect(world, light_list, 213, 343, 227, 332, 554, light);
    
    ObjectHandle box1 = object_transform(world, object_box(world, v3(0, 0, 0), v3(165, 330, 165), white),
        transform_t_euler(v3(265,0,295), 0, rad(15), 0));
    ObjectHandle box2 = object_transform(world, object_box(world, v3(0, 0, 0), v3(165, 165, 165), white),
        transform_t_euler(v3(130, 0, 65), 0, rad(-18), 0));

    // MaterialHandle glass = material_dielectric(world, 1.5f);
    // ObjectHandle sphere = object_sphere(world, v3(190, 90, 190), 90, glass);
    ObjectList bvh = object_list_init(&world->arena, 2);
    add_object_to_list(&bvh, box2);
    add_object_to_list(&bvh, box1);
    add_object_to_world(world, object_bvh_node(world, bvh.a, bvh.size));

    add_important_object(world, light_list);
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(278, 278, -800);
    Vec3 look_at = v3(278, 278, 0);
    f32 dtf = 10;
    f32 aperture = 0.f;
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(40), aperture, dtf, 0, 1);
}

void 
init_scene3(World *world, Image *image) {
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(478, 278, -600);
    Vec3 look_at = v3(278, 278, 0);
    f32 dtf = 10;
    f32 aperture = 0.f;
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(40), aperture, dtf, 0, 1);
    
    u64 temp_arena_size = MEGABYTES(1);
    MemoryArena temp_arena = memory_arena(malloc(temp_arena_size), temp_arena_size);
    
    MaterialHandle ground = material_lambertian(world, texture_solid(world, v3(0.48, 0.83, 0.53)));

    const int boxes_per_side = 20;
    ObjectList boxes_list = object_list_init(&temp_arena, boxes_per_side * boxes_per_side);
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            f32 w = 100.0;
            f32 x0 = -1000.0 + i*w;
            f32 z0 = -1000.0 + j*w;
            f32 y0 = 0.0;
            f32 x1 = x0 + w;
            f32 y1 = random_uniform(&global_entropy, 1, 101);
            f32 z1 = z0 + w;
            
            ObjectHandle object = object_box(world, v3(x0,y0,z0), v3(x1,y1,z1), ground);
            add_object_to_list(&boxes_list, object);
        }
    }
    add_object_to_world(world, object_bvh_node(world, boxes_list.a, boxes_per_side * boxes_per_side));

    MaterialHandle light = material_diffuse_light(world, texture_solid(world, v3s(7)), true);
    // add_xz_rect(world, world->obj_list, 123, 423, 147, 412, 554, light);
    
    ObjectHandle lights = object_list(world);
    add_xz_rect(world, lights, 123, 423, 147, 412, 554, light);
    add_object_to_world(world, lights);
    add_important_object(world, lights);
        
    add_object_to_world(world, object_sphere(world, v3(260, 150, 45), 50, material_dielectric(world, 1.5)));
    add_object_to_world(world, object_sphere(world, v3(0, 150, 145), 50, material_metal(world, texture_solid(world, v3(0.8, 0.8, 0.9)), 1)));

    MaterialHandle sphere_mat = material_lambertian(world, texture_solid(world, v3(0.7, 0.3, 0.1)));
    ObjectHandle sphere = object_sphere(world, v3(400, 400, 200), 50, sphere_mat);
    add_object_to_world(world, object_animated_transform(world, sphere, 0, 1, v3s(0), v3(30, 0, 0), QUAT4_IDENTITY, QUAT4_IDENTITY));

    ObjectHandle bound1 = object_sphere(world, v3(360,150,145), 70, material_dielectric(world, 1.5f));
    add_object_to_world(world, bound1);
    MaterialHandle smoke1_mat = material_isotropic(world, texture_solid(world, v3(0.2, 0.4, 0.9)));
    add_object_to_world(world, object_constant_medium(world, 0.2f, smoke1_mat, bound1));
    
    ObjectHandle bound2 = object_sphere(world, v3(0,0,0), 5000, material_dielectric(world, 1.5f));
    add_object(world, world->obj_list, bound2);
    MaterialHandle smoke2_mat = material_isotropic(world, texture_solid(world, v3s(1)));
    add_object(world, world->obj_list, object_constant_medium(world, 0.0001f, smoke2_mat, bound2));

    MaterialHandle emat = material_lambertian(world, texture_image(world, load_bmp("earth.bmp")));
    add_object(world, world->obj_list, object_sphere(world, v3(400,200,400), 100, emat));
    MaterialHandle marble = material_lambertian(world, texture_perlin(world, make_perlin(&world->arena, &global_entropy), 0.1f));  
    add_object_to_world(world, object_sphere(world, v3(220,280,300), 80, marble));

    MaterialHandle white = material_lambertian(world, texture_solid(world, v3(.73, .73, .73)));
    int ns = 1000;
    ObjectList spheres = object_list_init(&temp_arena, ns);
    for (int j = 0; j < ns; j++) {
        ObjectHandle sphere = object_sphere(world, v3add(v3(-100,270,395), random_vector(&global_entropy, 0, 165)), 10, white);
        add_object_to_list(&spheres, sphere);
    }
    add_object_to_world(world, object_bvh_node(world, spheres.a, ns));
    
    free(temp_arena.data);
}

void 
init_scene_test(World *world, Image *image) {
    MaterialHandle ground_mat = material_lambertian(world,
        texture_checkerboard3d(world,
        texture_solid(world, v3(0.2, 0.3, 0.1)),
        texture_solid(world, v3(0.9, 0.9, 0.9))));
    // MaterialHandle white = material_lambertian(world, texture_solid(world, v3s(1)));
    // MaterialHandle normal = material_lambertian(world, texture_normal(world));
    // MaterialHandle uv = material_lambertian(world, texture_uv(world));
    // MaterialHandle emat = material_lambertian(world, texture_image(world, load_bmp("earth.bmp")));
    // MaterialHandle mat = material_metal(world, texture_normal(world), 0.2);
    // MaterialHandle mat = material_plastic(world, texture_solid(world, v3(0.2, 0.2, 0.5)), 0.2);
    MaterialHandle mat = material_plastic(world, texture_solid(world, v3(0.2, 0.2, 0.5)), 0.1);
    MaterialHandle mat_met = material_metal(world, texture_solid(world, v3(0.2, 0.2, 0.5)), 0.2);
    // MaterialHandle mat = material_dielectric(world, GLASS_IR);

    add_object_to_world(world, object_sphere(world, v3(0, -1001, 0), 1000, ground_mat));
    // add_object_to_world(world, object_sphere(world, v3(1, 0, 0.5), 1, mat));
    ObjectHandle monkey = object_triangle_mesh_tt(world, load_obj("model.obj"), mat_met,
        transform_t_euler(v3(0, -0.5, 0), -rad(34), PI/4, 0));
    add_object_to_world(world, monkey);
    // add_object_to_world(world, add_poly_sphere(world, 1, 15, mat));
    // add_object_to_world(world, object_disk(world, v3(0, 0.0, 0), v3(0, 1, 0), 1, mat));
    // add_object_to_world(world, object_disk(world, v3(0, 2.5, 0), v3(0, 1, 0), 1.5, mat_met));
    
    // world->backgorund_color = v3(0.70, 0.80, 1.00);
    MaterialHandle light = material_diffuse_light(world, texture_solid(world, v3s(10)), true);
    ObjectHandle lights = object_list(world);
    // add_object(world, lights, object_sphere(world, v3(0, 3, 0), 1, light));
    add_object(world, lights, object_disk(world, v3(0, 2.2, 0), v3(0, -1, 0), 1, light));
    add_object_to_world(world, lights);
    add_important_object(world, lights);
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(13, 2, 5);
    Vec3 look_at = v3(0, 0.5, 0);
    f32 dtf = 10;
    f32 aperture = 0.0f;
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(20), aperture, dtf, 0, 1);
}
