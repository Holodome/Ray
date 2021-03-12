
void 
init_scene2(World *world, Image *image) {
    world->backgorund_color = v3(0.70, 0.80, 1.00);  
    MaterialHandle ground_mat = material_lambertian(world,
        texture_checkerboard3d(world,
        texture_solid(world, v3(0.2, 0.3, 0.1)),
        texture_solid(world, v3(0.9, 0.9, 0.9))));
    add_object_to_world(world, object_sphere(world, v3(0, -1000, 0), 1000, ground_mat));
    
    ObjectList bvh = object_list_init(&world->arena, 1000);
    for (i32 a = -11; 
         a < 11; 
         a++) {
        for (i32 b = -11;
             b < 11;
             b++) {
            f32 choose_mat = randomu(&rng);
            Vec3 center = v3(a + 0.9f * randomu(&rng),
                             0.2,
                             b + 0.9 * randomu(&rng));

            if (length(v3sub(center, v3(4, 0.2, 0))) > 0.9) {
                if (choose_mat < 0.8) {
                    Vec3 albedo = v3mul(random_vector(&rng, 0.1f, 1.0f), random_vector(&rng, 0.1f, 1.0f));
                    MaterialHandle mat = material_lambertian(world, texture_solid(world, albedo));       
                    add_object_to_list(&bvh, object_sphere(world, center, 0.2f, mat));
                } else if (choose_mat < 0.95) {
                    f32 roughness = random_uniform(&rng, 0, 0.5);
                    Vec3 albedo = random_vector(&rng, 0.5, 1);
                    // MaterialHandle mat = material_metal(world, texture_solid(world, albedo), fuzz, random_uniform(&rng, 0.5, 1));       
                    MaterialHandle mat = material_metal(world, roughness, texture_solid(world, albedo));       
                    add_object_to_list(&bvh, object_sphere(world, center, 0.2f, mat));
                } else {
                    MaterialHandle mat = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3s(1)));       
                    add_object_to_list(&bvh, object_sphere(world, center, 0.2f, mat));
                }
            }
        }
    }
    add_object_to_world(world, object_bvh_node(world, bvh.a, bvh.size));
    
    MaterialHandle di_m = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3(1, 1, 0)));       
    add_object_to_world(world, object_sphere(world, v3(0, 1, 0), 1, di_m));
    
    MaterialHandle la_m = material_lambertian(world, texture_solid(world, v3(0.4, 0.2, 0.1)));
    add_object_to_world(world, object_sphere(world, v3(-4, 1, 0), 1, la_m));
    
    // MaterialHandle me_m = material_plastic(world, 0, 1, 1.5, texture_solid(world, v3(0.7, 0.6, 0.5)), texture_solid(world, v3s(1)));       
    MaterialHandle me_m = material_metal(world, 0, texture_solid(world, v3(0.7, 0.6, 0.5)));
    add_object_to_world(world, object_sphere(world, v3(4, 1, 0), 1, me_m));
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;

    Vec3 look_from = v3(13, 2, 3);
    // Vec3 look_from = v3(2, 2, 3);
    Vec3 look_at = v3(0, 0, 0);
    f32 dtf = 10.0f;
    f32 aperture = 0.0f;
    // world->camera = camera_environment(look_from, look_at, v3(0, 1, 0), 0, 1);
    world->camera = camera_perspective(look_from, look_at, v3(0, 1, 0), aspect_ratio, rad(20), aperture, dtf, 0, 1);
}

void 
init_scene_bigger(World *world, Image *image) {
    world->backgorund_color = v3(0.70, 0.80, 1.00);  
    
    MaterialHandle sky = material_diffuse_light(world, texture_image(world, load_bmp("pano.bmp")), LightFlags_BothSided);
    add_object_to_world(world, object_sphere(world, v3(0, 0, 0), 1000, sky));
    
    MaterialHandle ground_mat = material_lambertian(world,
        texture_checkerboard3d(world,
        texture_solid(world, v3(0.2, 0.1, 0.3)),
        texture_solid(world, v3(0.9, 0.9, 0.9))));
    // add_object_to_world(world, object_sphere(world, v3(0, -1000, 0), 1000, ground_mat));
    
    MaterialHandle m0 = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3(1, 1, 1)));
    add_object_to_world(world, object_sphere(world, v3(0, 1, 0), 1, m0));
    MaterialHandle m1 = material_metal(world, 0, texture_solid(world, v3(0.7, 0.6, 0.5)));
    add_object_to_world(world, object_sphere(world, v3(-4, 1, 0), 1, m1));
    MaterialHandle m2 = material_lambertian(world, texture_image(world, load_bmp("earth.bmp")));
    add_object_to_world(world, object_sphere(world, v3(4, 1, 0), 1, m2));
    
    MaterialHandle lightm = material_diffuse_light(world, texture_solid(world, v3s(25)), 0);
    add_object_to_world(world, object_sphere(world, v3(0, 3, 3), 1, lightm));
    
    ObjectList bvh = object_list_init(&world->arena, 0);
    u32 obj_cnt = 100;
    for (u32 obj_idx = 0;
         obj_idx < obj_cnt;
         ++obj_idx) {
        f32 pos_rad = 10;
        Vec3 p;
        do {
            p = random_vector(&rng, -pos_rad, pos_rad);
            p.y = 0.0f;
        } while(length(v3sub(v3(4, 0, 0), p)) < 1 || length(v3sub(v3(-4, 0, 0), p)) < 1 || length(v3sub(v3(0, 0, 0), p)) < 1);
        
        MaterialHandle mat;
        f32 choose_mat = randomu(&rng);
        if (choose_mat < 0.5) {
            f32 r = randomu(&rng);
            // f32 r =0.1;
            Vec3 d = random_vector(&rng, 0, 1);
            mat = material_plastic(world, r, 1, 1.5, texture_solid(world, d), texture_solid(world, v3s(1)));
        } else {
            Vec3 d = random_vector(&rng, 0, 1);
            Vec3 t = random_vector(&rng, 0, 1);
            mat = material_dielectric(world, 0, 1, 1.5, texture_solid(world, d), texture_solid(world, t));
        }
        
        ObjectHandle obj;
        f32 choose_obj = randomu(&rng);
        if (choose_obj < 0.5) {
            obj = object_sphere(world, v3add(p, v3(0, 0.4, 0)), 0.4, mat);
        } else {
            obj = object_box(world, v3add(p, v3(-0.2, 0, -0.2)), v3add(p, v3(0.2, 0.4, 0.2)), mat);
        }
        add_object_to_list(&bvh, obj);
    }
    
    add_object_to_world(world, object_bvh_node(world, bvh.a, bvh.size));
    
    world->camera = camera_perspective(v3(20, 3, 6),  v3(0, 2, 0), v3(0, 1, 0), 
        (f32)image->w / (f32)image->h, rad(20), 0.0f, 10.0f, 0, 1);
}

void 
init_scene1(World *world, Image *image) {
    MaterialHandle marble = material_lambertian(world, texture_perlin(world, make_perlin(&world->arena, &rng), 4.0f));
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
    MaterialHandle white = material_lambertian(world, texture_solid(world, v3(0.73, 0.73, 0.73)));
    MaterialHandle red   = material_lambertian(world, texture_solid(world, v3(0.65, 0.05, 0.05)));
    MaterialHandle green = material_lambertian(world, texture_solid(world, v3(0.12, 0.45, 0.15)));
    MaterialHandle light = material_diffuse_light(world, texture_solid(world, v3s(15)), LightFlags_FlipFace);
    MaterialHandle mirror = material_metal(world, 0, texture_solid(world, v3s(1)));
    
    f32 m = 1 / 1.5f;
    add_yz_rect(world, world->obj_list, 0, 5.55 * m, 0, 5.55 * m, 5.55 * m, green);
    // add_yz_rect(world, world->obj_list, 0, 5.55 * m, -5.55 * m, 5.55 * m, 5.55 * m, mirror);
    add_yz_rect(world, world->obj_list, 0, 5.55 * m, 0, 5.55 * m, 0, red);
    add_xz_rect(world, world->obj_list, 2.13* m, 3.43* m, 2.27* m, 3.32* m, 5.54* m, light);
    // add_xz_rect(world, world->obj_list, 1.13 *m, 4.43*m, 1.27*m, 4.32*m, 5.54*m, light);
    add_xz_rect(world, world->obj_list, 0, 5.55 * m, 0, 5.55 * m, 0, white);
    add_xz_rect(world, world->obj_list, 0, 5.55 * m, 0, 5.55 * m, 5.55 * m, white);
    add_xy_rect(world, world->obj_list, 0, 5.55 * m, 0, 5.55 * m, 5.55 * m, white);
    // add_yz_rect(world, world->obj_list, 1.13 *m, 4.43*m, 1.27*m, 4.32*m, 5.54*m, mirror);
    
    add_xz_rect(world, world->important_objects, 2.13* m, 3.43* m, 2.27* m, 3.32* m, 5.54* m, light);
    // add_xz_rect(world, world->important_objects, 1.13 *m, 4.43*m, 1.27*m, 4.32*m, 5.54*m, light);
    
    // world->backgorund_color = v3s(1);
    
    // MaterialHandle mirror = material_mirror(world);
    // MaterialHandle mat = material_plastic(world, 0.0, 1, 1.5, texture_solid(world, v3(0.7, 0.6, 0.5)), texture_solid(world, v3s(1)));
    // MaterialHandle m1 = material_plastic(world, 0.0, 1, 1.5, texture_solid(world, v3(0, 0, 1)), texture_solid(world, v3s(1)));
    // MaterialHandle m2 = material_plastic(world, 0.0, 1, 1.5, texture_solid(world, v3(1, 0, 0)), texture_solid(world, v3s(1)));
    // MaterialHandle m1 = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3(1, 1, 0)));       
    // MaterialHandle m2 = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3(1, 0, 1)));       

    MaterialHandle mat = white;
    ObjectHandle box1 = object_transform(world, object_box(world, v3(0, 0, 0), v3(1.65 * m, 3.30 * m, 1.65 *m), mat),
        transform_t_euler(v3(2.65*m, 0, 2.95*m), 0, rad(15), 0));
    ObjectHandle box2 = object_transform(world, object_box(world, v3(0, 0, 0), v3(1.65*m, 1.65*m, 1.65*m), mat),
        transform_t_euler(v3(1.30*m, 0, 0.65*m), 0, rad(-18), 0));
    // ObjectList bvh = object_list_init(&world->arena, 2);
    // add_object_to_list(&bvh, box2);
    // add_object_to_list(&bvh, box1);
    add_object_to_world(world, box1);
    add_object_to_world(world, box2);
    
    // add_object_to_world(world, object_sphere(world, v3(400,100,280), 100, m1));
    // add_object_to_world(world, object_sphere(world, v3(150, 90, 190), 90, m2));
    // world->backgorund_color = v3s(1);
    // MaterialHandle monk_mot = material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3(0.2, 0.2, 1)));
    // MaterialHandle monk_mot = material_lambertian(world, texture_solid(world, v3(0.2, 0.2, 1)));
    // MaterialHandle monk_mot = material_lambertian(world, texture_normal(world));

    // add_object_to_world(world, object_transform(world, add_poly_sphere(world, 1, 10, normal), transform_t(v3(2.7 * m, 2.7 * m, 4.2 * m))));
    // ObjectHandle monkey = object_triangle_mesh_tt(world, load_obj("model.obj"), monk_mot, transform_tr(v3(2.7* m, 2.7* m, 3.5* m), q4euler(rad(-10), PI, 0)));
    // add_object_to_world(world, monkey);
    // add_important_object(world, monkey);

    // MaterialHandle glass = material_dielectric(world, 1.5f);
    // ObjectHandle sphere = object_sphere(world, v3(190, 90, 190), 90, glass);
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from = v3(2.78 *m, 2.78*m, -8.00*m);
    Vec3 look_at = v3(2.78*m, 2.78*m, 0);
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
    
    MaterialHandle ground = material_lambertian(world, texture_solid(world, v3(0.48, 0.83, 0.53)));

    const int boxes_per_side = 20;
    ObjectList boxes_list = object_list_init(&world->arena, boxes_per_side * boxes_per_side);
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            f32 w = 100.0;
            f32 x0 = -1000.0 + i*w;
            f32 z0 = -1000.0 + j*w;
            f32 y0 = 0.0;
            f32 x1 = x0 + w;
            f32 y1 = random_uniform(&rng, 1, 101);
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
        
    // add_object_to_world(world, object_sphere(world, v3(260, 150, 45), 50, material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3s(1)))));
    // add_object_to_world(world, object_sphere(world, v3(0, 150, 145), 50, material_metal(world, 0.3, texture_solid(world, v3(0.8, 0.8, 0.9)))));

    MaterialHandle sphere_mat = material_lambertian(world, texture_solid(world, v3(0.7, 0.3, 0.1)));
    ObjectHandle sphere = object_sphere(world, v3(400, 400, 200), 50, sphere_mat);
    add_object_to_world(world, object_animated_transform(world, sphere, 0, 1, v3s(0), v3(30, 0, 0), QUAT4_IDENTITY, QUAT4_IDENTITY));

    // ObjectHandle bound1 = object_sphere(world, v3(360,150,145), 70, material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3s(1))));
    // add_object_to_world(world, bound1);
    // MaterialHandle smoke1_mat = material_isotropic(world, texture_solid(world, v3(0.2, 0.4, 0.9)));
    // add_object_to_world(world, object_constant_medium(world, 0.2f, smoke1_mat, bound1));
    
    // ObjectHandle bound2 = object_sphere(world, v3(0,0,0), 5000, material_dielectric(world, 0, 1, 1.5, texture_solid(world, v3s(1)), texture_solid(world, v3s(1))));
    // add_object(world, world->obj_list, bound2);
    // MaterialHandle smoke2_mat = material_isotropic(world, texture_solid(world, v3s(1)));
    // add_object(world, world->obj_list, object_constant_medium(world, 0.0001f, smoke2_mat, bound2));

    MaterialHandle emat = material_lambertian(world, texture_image(world, load_bmp("earth.bmp")));
    add_object(world, world->obj_list, object_sphere(world, v3(400,200,400), 100, emat));
    MaterialHandle marble = material_lambertian(world, texture_perlin(world, make_perlin(&world->arena, &rng), 0.1f));  
    add_object_to_world(world, object_sphere(world, v3(220,280,300), 80, marble));

    MaterialHandle white = material_lambertian(world, texture_solid(world, v3(.73, .73, .73)));
    int ns = 1000;
    ObjectList spheres = object_list_init(&world->arena, ns);
    for (int j = 0; j < ns; j++) {
        ObjectHandle sphere = object_sphere(world, v3add(v3(-100,270,395), random_vector(&rng, 0, 165)), 10, white);
        add_object_to_list(&spheres, sphere);
    }
    // add_object_to_world(world, object_bvh_node(world, spheres.a, ns));
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
    // MaterialHandle mat = material_plastic(world, texture_solid(world, v3(0.2, 0.2, 0.5)), 0.1, 1);
    MaterialHandle mat_met = material_metal(world, 0.2, texture_solid(world, v3(0.2, 0.2, 0.5)));
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

void 
init_scene_test_light(World *world, Image *image) {
    // MaterialHandle light = ;
    MaterialHandle floor = material_lambertian(world, texture_solid(world, v3(0.4, 0.04, 0.04)));
#if 1
    MaterialHandle plate1 = material_metal(world, 0.010, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate2 = material_metal(world, 0.050, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate3 = material_metal(world, 0.100, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate4 = material_metal(world, 0.150, texture_solid(world, v3(0.2, 0.2, 0.2)));
#else 
    MaterialHandle plate1 = material_metal(world, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate2 = material_metal(world, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate3 = material_metal(world, texture_solid(world, v3(0.2, 0.2, 0.2)));
    MaterialHandle plate4 = material_metal(world, texture_solid(world, v3(0.2, 0.2, 0.2)));
#endif     

    add_xz_rect(world, world->obj_list, -10, 10, -10, 10, -4.146150112, floor);
    add_xy_rect(world, world->obj_list, -10, 10, -10, 10, -2, floor);
    
    add_rect(world, world->obj_list, v3(4 ,-2.706510067, 0.2560899854),
                                     v3(4 ,-2.08375001, -0.5263199806),
                                     v3(-4, -2.08375001, -0.5263199806),
                                     v3(-4, -2.706510067, 0.2560899854), plate1);
    
    add_rect(world, world->obj_list, v3(4, -3.288249969, 1.369719982),
                                     v3(4, -2.838560104, 0.4765399992),
                                     v3(-4, -2.838560104, 0.4765399992),
                                     v3(-4, -3.288249969, 1.369719982), plate2);
                                     
    add_rect(world, world->obj_list, v3(4, -3.730959892, 2.700459957),
                                     v3(4, -3.433779955, 1.745640039),
                                     v3(-4, -3.433779955, 1.745640039),
                                     v3(-4, -3.730959892, 2.700459957), plate3);
    
    add_rect(world, world->obj_list, v3(4, -3.996150017, 4.066699982),
                                     v3(4, -3.820689917, 3.082210064),
                                     v3(-4, -3.820689917, 3.082210064),
                                     v3(-4, -3.996150017, 4.066699982), plate4);
    
    ObjectHandle sphere1 = add_object_to_world(world, object_sphere(world, v3(-3.75, 0.0, 0), 0.05,
         material_diffuse_light(world, texture_solid(world, v3s(901)), 0)));
    ObjectHandle sphere2 = add_object_to_world(world, object_sphere(world, v3(-1.25, 0.0, 0), 0.1,
         material_diffuse_light(world, texture_solid(world, v3s(100)), 0)));
    ObjectHandle sphere3 = add_object_to_world(world, object_sphere(world, v3(1.00, 0.0, 0), 0.3, 
        material_diffuse_light(world, texture_solid(world, v3s(11.1)), 0)));
    ObjectHandle sphere4 = add_object_to_world(world, object_sphere(world, v3(3.75, 0.0, 0), 0.9,
         material_diffuse_light(world, texture_solid(world, v3s(5.2)), 0)));
    add_important_object(world, sphere1);
    add_important_object(world, sphere2);
    add_important_object(world, sphere3);
    add_important_object(world, sphere4);
    
    f32 aspect_ratio = (f32)image->w / (f32)image->h;
    Vec3 look_from =  v3(0, 2, 15);
    Vec3 look_at = v3(0, 1.69522, 14.0476);
    Vec3 v_up = v3(0, 1, 0);
    // Vec3 v_up = v3(0, 1, 0);
    f32 dtf = 1e6;
    f32 aperture = 0.0f;
    world->camera = camera_perspective(look_from, look_at, v_up, aspect_ratio, rad(36.7), aperture, dtf, 0, 1);
}
