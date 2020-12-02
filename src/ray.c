#include "ray.h"
#include "ray_thread.h"

#include "ray_thread.c"
#include "trace.c"
#include "scenes.c"

RandomSeries global_entropy = { 546674572 };

static bool 
render_tile(RenderWorkQueue *queue) {
    u64 work_order_index = atomic_add64(&queue->next_order_index, 1);
    if (work_order_index >= queue->order_count) {
        return false;
    }
    
    RenderWorkOrder *order = queue->orders + work_order_index;
    
    u64 bounce_count = 0;
    u64 ray_triangle_collision_tests = 0;
    u64 object_collision_tests = 0;
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
                Vec3 sample_color = ray_cast(queue->world, ray, &order->entropy, &cast_data);
                bounce_count += cast_data.bounce_count;
                ray_triangle_collision_tests += cast_data.ray_triangle_collision_tests;
                object_collision_tests += cast_data.object_collision_tests;
                
                f32 color_multiplier = 1.0f / (f32)SAMPLES_PER_PIXEL;
                pixel_color = v3add(pixel_color, v3muls(sample_color, color_multiplier));
            }
            
            f32 r = linear1_to_srgb1(pixel_color.r);
            f32 g = linear1_to_srgb1(pixel_color.g);
            f32 b = linear1_to_srgb1(pixel_color.b);
            *pixel++ = rgba_pack_4x8_linear1(b, g, r, 1.0f);     
        }        
    }
    
    atomic_add64(&queue->orders_done, 1);
    atomic_add64(&queue->total_bounce_count, bounce_count);
    atomic_add64(&queue->total_ray_triangle_collision_tests, ray_triangle_collision_tests);
    atomic_add64(&queue->object_collision_tests, object_collision_tests);
    
    return true;
}

static THREAD_PROC_SIGNATURE(render_thread_proc) {
    RenderWorkQueue *queue = param;
    
    while (render_tile(queue)) { }
    
    exit_thread();
    return 0;
}

static void
parse_command_line_arguments(u32 argc, char **argv, RaySettings *s) {
    u32 cursor = 1;
    
    while (cursor < argc) {
        char *arg = argv[cursor];

#define CHECK_HAS_ENOUGH_ARGS_OR_ERROR(_arg_count)                                          \
    if (cursor + _arg_count >= argc) {                                                      \
        fprintf(stderr, "[ERROR] Argument %s takes exactly 1 positional argument\n", arg);  \
        break;                                                                              \
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
        } else {
            fprintf(stderr, "[ERROR] Unknown argument %s\n", arg);
            break;
        }
    }
}

int 
main(int argc, char **argv) {
    RaySettings settings = {0};
    
    u32 image_w = 480 * 2;
    u32 image_h = 480 * 2;
    
    Image output_image = make_image_for_writing(image_w, image_h);
    
    World world = {0};
    world.arena.data_capacity = MEGABYTES(16);
    world.arena.data = malloc(world.arena.data_capacity);
    init_scene3(&world, &output_image);
    char bytes_buffer[32];
    format_bytes(bytes_buffer, sizeof(bytes_buffer), world.arena.data_size);
    printf("Scene memory taken: %s\n", bytes_buffer);
    
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
            
#define MAKE_SEED(a, b, c, d) ((a) * 13998 + (b) * 39224 + (c) * 60918 + (d) * 14319)
            u32 seed = MAKE_SEED(tile_count_x, tile_count_y, tile_x, tile_y);
            order->entropy.state = seed;
        }           
    }
    assert(cursor == render_queue.order_count);
    
    printf("Scene object count: %llu\n", world.objects_size);
    printf("Scene texture count: %llu\n", world.textures_size);
    printf("Scene material count: %llu\n", world.materials_size);
    
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
        } else {
            _mm_pause();
        }
    }
    printf("\nRaycasting done\n");
    
    clock_t end_clock = clock();
    clock_t time_elapsed = end_clock - start_clock;
    
    char time_string[64];
    format_time_ms(time_string, sizeof(time_string), (u64)time_elapsed);
    
    char number_buffer[100];
    printf("Raycasting time: %s\n", time_string);
    printf("Pixel count: %u\n", output_image.w * output_image.h);
    printf("Samples per pixel: %llu\n", SAMPLES_PER_PIXEL);
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), SAMPLES_PER_PIXEL * output_image.w * output_image.h);
    printf("Primary ray count: %s\n", number_buffer);
    printf("Perfomace: %fms/primary ray\n", (f64)time_elapsed / (f64)(SAMPLES_PER_PIXEL * output_image.w * output_image.h));
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.total_bounce_count);
    printf("Total bounces: %s\n", number_buffer);
    printf("Perfomance: %fms/bounce\n", (f64)time_elapsed / (f64)render_queue.total_bounce_count);
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.total_ray_triangle_collision_tests);
    printf("Triangle collision tests: %s\n", number_buffer);
    format_number_with_thousand_separators(number_buffer, sizeof(number_buffer), render_queue.object_collision_tests);
    printf("Object collision tests: %s\n", number_buffer);
    printf("Average bounce count per ray: %f\n", (f64)render_queue.total_bounce_count / (f64)(SAMPLES_PER_PIXEL * output_image.w * output_image.h));
    
    char *out = "out.bmp";
    image_save(&output_image, out);
    char command[32] = {0};
    snprintf(command, sizeof(command), "start %s", out);
    system(command);
    
    printf("Exited successfully\n");
    return 0;
}
