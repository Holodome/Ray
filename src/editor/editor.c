#include "editor/editor.h"

#include "lib/ray_math.c"
#include "lib/sys.c"
#include "lib/memory_pool.c"

#include "ray/ray_tracer.c"
#include "ray/scene_file.c"

#include "editor/renderer/opengl.c"
#include "editor/ui.c"

#include "font.c"
#include "image.c"

static void 
update_time(Input *input)
{
    clock_t current_time = clock();
    if (input->frame_start_time)
    {
        clock_t cldt = current_time - input->frame_start_time;
        f32 dts = (f32)cldt / (f32)CLOCKS_PER_SEC;
        input->dt = dts;
        input->time += dts;
    }
    input->frame_start_time = current_time;
}

static void
render_scene(OpenGLRenderer *renderer, Scene *scene)
{
    for (u32 rect_index = 0;
        rect_index < scene->rect_count;
        ++rect_index)
    {
        Rect rect_ = scene->rects[rect_index];
        XYRect rect = rect_.xy;
        
        u32 a_index = 0;
        u32 b_index = 1;
        if (rect_.type == RectType_XZ)
        {
            b_index = 2;
        }
        else if (rect_.type == RectType_YZ)
        {
            a_index = 1;
            b_index = 2;
        }
        u32 c_index = 3 - a_index - b_index;
        
        Vec3 v00, v01, v10, v11;
        
        v00.e[a_index] = rect.x0;       
        v00.e[b_index] = rect.y0;       
        v00.e[c_index] = rect.k;       
        
        v01.e[a_index] = rect.x0;       
        v01.e[b_index] = rect.y1;       
        v01.e[c_index] = rect.k;
        
        v10.e[a_index] = rect.x1;       
        v10.e[b_index] = rect.y0;       
        v10.e[c_index] = rect.k;
        
        v11.e[a_index] = rect.x1;       
        v11.e[b_index] = rect.y1;       
        v11.e[c_index] = rect.k;
        
        Material material = scene->materials[rect.mat_index];
        
        Vec4 color;
        color.xyz = material.texture.solid_color;
        color.w = 1.0f;
        push_quad(renderer, v00, v01, v10, v11, color, color, color, color, 
                  vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), empty_texture);
    }
}

int 
main(int argc, char **argv)
{
	printf("Editor is started\n");
	
    struct SysWindow *window = sys_create_window(1280, 720);
	Input input = {0};
    struct SysGLCTX *ctx = sys_init_opengl(window, 1);
	struct OpenGLRenderer *opengl = sys_create_renderer(ctx);
	opengl_init(opengl);
    
    Font font = {0};
    font_load(&font, "data/arial.ttf", 32.0f, opengl);
    ui_init(opengl, &input, font);
    
    ImageU32 image_view = { 600, 600 };
    
    Editor editor = {0};
    editor.current_scene_filename = "data/cornell.ray_scene";
    scene_init_from_file(&editor.ray_tracer_scene, &image_view, editor.current_scene_filename);
    
    f32 half_width = (f32)image_view.width * 0.5f;
    f32 half_height = (f32)image_view.height * 0.5f;
    
    // Mat4x4 projection = mat4x4_orthographic3d(-half_width, half_width, -half_height, half_height, 0.f, 1.0f);
    Mat4x4 projection = mat4x4_perspective(PI / 3.0f, half_width / half_height, 0.1f, 50.0f);
    Mat4x4 view       = mat4x4_identity();
    
    Vec3 camera_pos = vec3(0, -16, 0);
    Vec3 camera_rotation = vec3(0, 0, 0);
    
    for(;;) 
    {
        sys_update_input(window, &input);
        update_time(&input);
        
        if (input.is_quit_requested)
        {
			printf("Quit is requested\n");
            break;
        }
        
		Vec3 clear_color = vec3(0.2f, 0.2f, 0.2f);
        opengl_begin_frame(opengl, input.window_size, clear_color);
        ui_begin_frame();
        
        // Move camera
        f32 speed_multiplier = (is_key_pressed(input.keys[Key_LeftShift]) ? 2.0f : 1.0f);

        f32 speed = 0;
        if (is_key_pressed(input.keys[Key_W]))
            speed = 0.1f * speed_multiplier;
        else if (is_key_pressed(input.keys[Key_S]))
            speed = -0.1f * speed_multiplier;
        camera_pos.x += speed *  sin32(camera_rotation.y);
        camera_pos.z += speed * -cos32(camera_rotation.y);

        speed = 0;
        if (is_key_pressed(input.keys[Key_A]))
            speed = -0.1f * speed_multiplier;
        else if (is_key_pressed(input.keys[Key_D]))
            speed = 0.1f * speed_multiplier;
        camera_pos.x += speed *  sin32(camera_rotation.y + HALF_PI);
        camera_pos.z += speed * -cos32(camera_rotation.y + HALF_PI);

        if (is_key_pressed(input.keys[Key_Z])) camera_pos.y += 0.2f * speed_multiplier;
        if (is_key_pressed(input.keys[Key_X])) camera_pos.y -= 0.2f * speed_multiplier;
        // Rotate camera
        if (is_key_pressed(input.keys[Key_MouseLeft]))
        {
            camera_rotation.y += (input.mouse_delta_x / input.window_size.x) * 5.0f;
            camera_rotation.x += (input.mouse_delta_y / input.window_size.y) * 5.0f;
        }
        
        view = mat4x4_identity();
        view = mat4x4_mul(view, mat4x4_rotate(camera_rotation.x, vec3(1, 0, 0)));
        view = mat4x4_mul(view, mat4x4_rotate(camera_rotation.y, vec3(0, 1, 0)));
        view = mat4x4_mul(view, mat4x4_translate(vec3_neg(camera_pos)));
        // view = mat4x4_mul(view, mat4x4_rotate(camera_rotation.y, vec3(0, 0, 1)));
        // view = mat4x4_look_at(camera_pos, vec3(0, 0, 0));
        push_projection(opengl, projection);
        push_view(opengl, view);
        // push_projection(opengl, view);
        // push_view(opengl, projection);
        // push_quadr(opengl, rect2(-100, -100, 100, 100), vec4(1, 0, 0, 1), empty_texture);
        render_scene(opengl, &editor.ray_tracer_scene);
        pop_projection(opengl);
        pop_view(opengl);
        
        ui_window("Window", rect2_point_size(300, 300, 200, 200), 0);
        ui_text("Hello some text");
        ui_text_f("Camera pos %f %f %f", camera_pos.x, camera_pos.y, camera_pos.z);
        ui_text_f("Camera rot %f %f %f", camera_rotation.x, camera_rotation.y, camera_rotation.z);
        
        ui_end_window();
            
		ui_end_frame();
        opengl_end_frame(opengl);
        sys_swap_buffers(window, ctx, 1);
    }
	printf("Editor exited\n");
}