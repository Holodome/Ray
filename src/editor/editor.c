#include "editor/editor.h"

#include "ray_math.c"
#include "sys.c"
#include "editor/renderer/opengl.c"

int 
main(int argc, char **argv)
{
	printf("Editor is started\n");
	
    struct SysWindow *window = sys_create_window(1280, 720);
    struct SysGLCTX  *ctx    = sys_init_opengl(window, 1);
	
	struct OpenGLRenderer *opengl = sys_create_renderer(ctx);
	opengl_init(opengl);
	
	Input input = { 0 };
    for(;;) 
    {
        sys_update_input(window, &input);
        if (input.is_quit_requested)
        {
			printf("Quit is requested\n");
            break;
        }
		
		Vec3 clear_color = vec3(0.2f, 0.2f, 0.2f);
        opengl_begin_frame(opengl, input.window_size, clear_color);
        
		push_quadr(opengl, rect2_point_size(0, 0, 300, 300), vec4(1, 0, 0, 1), empty_texture);
		
        opengl_end_frame(opengl);
        sys_swap_buffers(window, ctx, 1);
    }
	printf("Editor exited\n");
}