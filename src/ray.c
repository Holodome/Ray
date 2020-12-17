#include "ray.h"
#include "ray_thread.h"

#include "ray_thread.c"
#include "trace.c"
#include "scenes.c"

RandomSeries global_entropy = { 546674573 };

bool 
render_tile(RenderWorkQueue *queue) {
    u64 work_order_index = atomic_add64(&queue->next_order_index, 1);
    if (work_order_index >= queue->order_count) {
        return false;
    }
    
    RenderWorkOrder *order = queue->orders + work_order_index;
    u32 samples = queue->samples_per_pixel;
    u32 bounces = queue->max_bounce_count;
    
    RayCastStatistics tile_stats = {0};
    for (u32 y = order->y_min;
         y < order->y_max;
         ++y) {
        u32 *pixel = image_get_pixel_pointer(queue->output, order->x_min, y);
             
        for (u32 x = order->x_min;
             x < order->x_max;
             ++x) {
            Vec3 pixel_color = v3(0, 0, 0);
            
            f32 color_multiplier = 1.0f / (f32)samples;
            for (u32 sample_index = 0;
                sample_index < samples;
                ++sample_index) {
                f32 u = ((f32)x + random(&order->entropy)) / (f32)queue->output->w;
                f32 v = ((f32)y + random(&order->entropy)) / (f32)queue->output->h;
                Ray ray = camera_make_ray(&queue->world->camera, &order->entropy, u, v);
                
                RayCastData data;
                data.entropy = &order->entropy;
                data.arena = &order->arena;
                data.stats = &tile_stats;
                
                Vec3 sample_color = ray_cast(queue->world, ray, bounces, data);
                // Remove NaNs
                if (!isfinite(sample_color.r)) { sample_color.r = 0; }
                if (!isfinite(sample_color.g)) { sample_color.g = 0; }
                if (!isfinite(sample_color.b)) { sample_color.b = 0; }
                
            
                pixel_color = v3add(pixel_color, v3muls(sample_color, color_multiplier));
            }
            
            pixel_color.r = saturate(pixel_color.r);
            pixel_color.g = saturate(pixel_color.g);
            pixel_color.b = saturate(pixel_color.b);
            f32 r = linear1_to_srgb1(pixel_color.r);
            f32 g = linear1_to_srgb1(pixel_color.g);
            f32 b = linear1_to_srgb1(pixel_color.b);
            *pixel++ = rgba_pack_4x8_linear1(b, g, r, 1.0f);     
        }        
    }
    
    atomic_add64(&queue->orders_done, 1);
    atomic_add64(&queue->stats.bounce_count, tile_stats.bounce_count);
    atomic_add64(&queue->stats.ray_triangle_collision_tests, tile_stats.ray_triangle_collision_tests);
    atomic_add64(&queue->stats.ray_triangle_collision_test_succeses, tile_stats.ray_triangle_collision_test_succeses);
    atomic_add64(&queue->stats.object_collision_tests, tile_stats.object_collision_tests);
    atomic_add64(&queue->stats.object_collision_test_successes, tile_stats.object_collision_test_successes);
    atomic_add64(&queue->stats.russian_roulette_terminated_bounces, tile_stats.russian_roulette_terminated_bounces);
    
    return true;
}

static THREAD_PROC_SIGNATURE(render_thread_proc) {
    RenderWorkQueue *queue = param;
    
    while (render_tile(queue)) { }
    
    exit_thread();
    return 0;
}

void 
init_render_queue(RenderWorkQueue *queue, Image *image, World *world,
                  u32 tile_w, u32 tile_h, u32 samples_per_pixel, u32 max_bounce_count) {
    memset(queue, 0, sizeof(*queue));
    // Ceil integer division
    u32 tile_count_x = (image->w + tile_w - 1) / tile_w;
    u32 tile_count_y = (image->h + tile_h - 1) / tile_h;
    u32 tile_count = tile_count_x * tile_count_y;
    
    queue->output = image;
    queue->world = world;
    queue->samples_per_pixel = samples_per_pixel;
    queue->max_bounce_count = max_bounce_count;
    queue->order_count = tile_count;
    queue->orders = malloc(sizeof(RenderWorkOrder) * queue->order_count);
    
    u32 cursor = 0;
    for (u32 tile_y = 0;
         tile_y < tile_count_y;
         ++tile_y) {
        u32 y_min = tile_y * tile_h;
        u32 y_max = y_min + tile_h;
        if (y_max > image->h) {
            y_max = image->h;
        }
        
        for (u32 tile_x = 0;
             tile_x < tile_count_x;
             ++tile_x) {
            u32 x_min = tile_x * tile_w;
            u32 x_max = x_min + tile_w;
            if (x_max > image->w) {
                x_max = image->w;
            }
            
            RenderWorkOrder *order =queue->orders + cursor++;
            assert(cursor <= queue->order_count);
            
            order->x_min = x_min;
            order->x_max = x_max;
            order->y_min = y_min;
            order->y_max = y_max;
            
#define MAKE_SEED(a, b, c, d) ((a) * 13998 + (b) * 39224 + (c) * 60918 + (d) * 14319)
            u32 seed = MAKE_SEED(tile_count_x, tile_count_y, tile_x, tile_y);
            order->entropy.state = seed;
        }           
    }
    assert(cursor == queue->order_count);
}

static void
parse_command_line_arguments(u32 argc, char **argv, RaySettings *s) {
    u32 cursor = 1;
    
    while (cursor < argc) {
        char *arg = argv[cursor];

#define CHECK_HAS_ENOUGH_ARGS_OR_ERROR(_arg_count)                                                                                       \
    if (cursor + _arg_count >= argc) {                                                                                                   \
        fprintf(stderr, "[ERROR] Argument %s takes exactly " #_arg_count " positional argument%c\n", arg, _arg_count != 1 ? 's' : ' ');  \
        break;                                                                                                                           \
    }       
    
        if (!strcmp(arg, "-out")) {
            CHECK_HAS_ENOUGH_ARGS_OR_ERROR(1);
            
            char *out_file = argv[cursor + 1];
            s->image_filename = out_file;
            
            cursor += 2;
        } else if (!strcmp(arg, "-size")) {
            CHECK_HAS_ENOUGH_ARGS_OR_ERROR(2);
            
            u32 w = atoi(argv[cursor + 1]);
            u32 h = atoi(argv[cursor + 2]);
            s->image_w = w;
            s->image_h = h;
            
            cursor += 3;
        } else if (!strcmp(arg, "-spp")) {
            CHECK_HAS_ENOUGH_ARGS_OR_ERROR(1);
            
            u32 v = atoi(argv[cursor + 1]);
            s->samples_per_pixel = v;
            
            cursor += 2;
        } else if (!strcmp(arg, "-mbc")) {
            CHECK_HAS_ENOUGH_ARGS_OR_ERROR(1);
            
            u32 v = atoi(argv[cursor + 1]);
            s->max_bounce_count = v;
            
            cursor += 2;
        } else if (!strcmp(arg, "-threads")) {
            CHECK_HAS_ENOUGH_ARGS_OR_ERROR(1);
            
            u32 v = atoi(argv[cursor + 1]);
            s->thread_count = v;
            
            cursor += 2;
        } else if (!strcmp(arg, "-open")) {
            s->open_image_after_done = true;
            ++cursor;
        } else {
            fprintf(stderr, "[ERROR] Unknown argument %s\n", arg);
            break;
        }
    }
}

int 
main(int argc, char **argv) {
#if RAY_INTERNAL 
    printf("RUNNING DEUBG BUILD\n");
#endif 
    
    RaySettings s = {0};
    s.image_w = 480;
    s.image_h = 480;
    s.image_filename = "out.bmp";
    s.samples_per_pixel = 32;
    s.max_bounce_count = 16;
    s.open_image_after_done = true;
    s.thread_count = 6;
    s.tile_w = 64;
    s.tile_h = 6;
    parse_command_line_arguments(argc, argv, &s);
    
    Image output_image = make_image_for_writing(s.image_w, s.image_h);
    
    // Initialize world
    World world;
    world_init(&world);
    init_cornell_box(&world, &output_image);
    // Print world information    
    char bytes_buffer[32];
    format_bytes(bytes_buffer, sizeof(bytes_buffer), world.arena.data_size);
    printf("Scene memory taken: %s\n", bytes_buffer);
    format_bytes(bytes_buffer, sizeof(bytes_buffer), world.arena.peak_size);
    printf("Scene memory peak size: %s\n", bytes_buffer);
    
    printf("Scene object count: %llu\n", world.objects_size);
    printf("Scene texture count: %llu\n", world.textures_size);
    printf("Scene material count: %llu\n", world.materials_size);
    printf("Thread count: %u\n", s.thread_count);
    printf("Image size: %ux%u\n", s.image_w, s.image_h);
    printf("Samples per pixel: %u\n", s.samples_per_pixel);
    printf("Max bounce count: %u\n", s.max_bounce_count);
    printf("Use importance sampling: %s\n", bool_to_cstring(world.has_importance_sampling));
    // Initialize multihtreaded work queue
    RenderWorkQueue render_queue;
    init_render_queue(&render_queue, &output_image, &world,
                      s.tile_w, s.tile_h, s.samples_per_pixel, s.max_bounce_count);
    
    printf("Start raycasting\n");
    clock_t start_clock = clock();
    
    // Create threads, they start working immediately
    for (u32 core_index = 1;
         core_index < s.thread_count;
         ++core_index) {
        create_thread(render_thread_proc, &render_queue);
    }
    
    // Keep maint thread busy
    while (render_queue.orders_done < render_queue.order_count) {
        if (render_tile(&render_queue)) {
            f32 percent = (f32)render_queue.orders_done / (f32)render_queue.order_count;
            printf("\rRaycasting %u%%", (u32)roundf(percent * 100));
            fflush(stdout);
        } 
    }
    printf("\nRaycasting done\n");
    
    clock_t end_clock = clock();
    clock_t elapsed = end_clock - start_clock;
    u64 time_elapsed = (u64)(elapsed * 1000 / CLOCKS_PER_SEC);
    
    char time_string[64];
    format_time_ms(time_string, sizeof(time_string), time_elapsed);
    printf("Raycasting time: %s\n", time_string);
    printf("Pixel count: %u\n", output_image.w * output_image.h);
    char number_buffer[100];
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), s.samples_per_pixel * output_image.w * output_image.h);
    printf("Primary ray count: %s\n", number_buffer);
    u64 primary_ray_count = s.samples_per_pixel * output_image.w * output_image.h;
    printf("Perfomace: %fms/primary ray\n", (f64)time_elapsed / (f64)primary_ray_count);
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.stats.bounce_count);
    printf("Total bounces: %s\n", number_buffer);
    printf("Perfomance: %fms/bounce\n", (f64)time_elapsed / (f64)render_queue.stats.bounce_count);
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.stats.ray_triangle_collision_tests);
    printf("Triangle collision tests: %s\n", number_buffer);
    printf("Triangle collision tests failed: %.2f%%\n", 100.0f * (1.0 - (f64)render_queue.stats.ray_triangle_collision_test_succeses / (f64)render_queue.stats.ray_triangle_collision_tests));
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.stats.object_collision_tests);
    printf("Object collision tests: %s\n", number_buffer);
    printf("Object collision tests failed: %.2f%%\n", 100.0f * (1.0 - (f64)render_queue.stats.object_collision_test_successes / (f64)render_queue.stats.object_collision_tests));
    printf("Average bounce count per ray: %f\n", (f64)render_queue.stats.bounce_count / (f64)(s.samples_per_pixel * output_image.w * output_image.h));
    printf("Russian rouletted terminated bounces: %llu (%.2f%%)\n", render_queue.stats.russian_roulette_terminated_bounces, (f64)render_queue.stats.russian_roulette_terminated_bounces / (f64)primary_ray_count);
    
    char *out = s.image_filename;
    image_save(&output_image, out);
    
    if (s.open_image_after_done) {
        char command[32];
        snprintf(command, sizeof(command), "start %s", out);
        system(command);
    }
    
    printf("Exited successfully\n");
    return 0;
}
