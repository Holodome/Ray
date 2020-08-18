#include "editor/editor.h"

#include "lib/ray_math.c"
#include "lib/sys.c"
#include "lib/memory_pool.c"

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

int 
main(int argc, char **argv)
{
	printf("Editor is started\n");
	
    struct SysWindow *window = sys_create_window(1280, 720);
    struct SysGLCTX  *ctx    = sys_init_opengl(window, 1);
	
	struct OpenGLRenderer *opengl = sys_create_renderer(ctx);
	opengl_init(opengl);
	
	Input input = { 0 };
    
    Font font = { 0 };
    font_load(&font, "data/arial.ttf", 32.0f, opengl);
    ui_init(opengl, &input, font);
    
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
        
        ui_window("Window", rect2_point_size(300, 300, 200, 200), UIWindowFlags_Default, 0);
        ui_text("Hello some text");
        
        ui_end_window();
            
		ui_end_frame();
        opengl_end_frame(opengl);
        sys_swap_buffers(window, ctx, 1);
    }
	printf("Editor exited\n");
}