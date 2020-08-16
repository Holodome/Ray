
#define WIN32_LEAD_AND_MEAN
#include <windows.h>

#include <intrin.h>

#include "editor/renderer/opengl.h"
#include "thirdparty/wgl.h"
#include "thirdparty/wglext.h"

typedef struct SysWindow
{
	HWND hwnd;
} SysWindow;

typedef struct SysGLCTX
{
	PFNWGLCREATECONTEXTPROC wglCreateContext;
	PFNWGLDELETECONTEXTPROC wglDeleteContext;
	PFNWGLGETPROCADDRESSPROC wglGetProcAddress;
	PFNWGLGETCURRENTDCPROC wglGetCurrentDC;
	PFNWGLMAKECURRENTPROC wglMakeCurrent;
	PFNWGLSWAPLAYERBUFFERSPROC wglSwapLayerBuffers;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;
	
	HMODULE opengl_dll;
} SysGLCTX;

//
//
//

u32 sys_get_processor_count(void)
{
	SYSTEM_INFO info = {0};
	GetSystemInfo(&info);
	u32 result = info.dwNumberOfProcessors;
	return result;
}

Thread
sys_create_thread(ThreadProc *proc, void *param)
{
	Thread result = {0};
	
	DWORD thread_id;
	HANDLE thread_handle = CreateThread(0, 0, proc, param, 0, &thread_id);
	CloseHandle(thread_handle);
	
	result.id = thread_id;
	
	return result;
}

void sys_exit_thread(void)
{
	ExitThread(0);
}

inline u64
atomic_add64(volatile u64 *value, u64 addend)
{
	u64 result = InterlockedExchangeAdd64((volatile long long *)value, addend);
	return result;
}

static LRESULT WINAPI
win32_main_window_proc(HWND window, UINT message,
					   WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	switch (message)
	{
		case WM_QUIT:
		case WM_DESTROY:
		case WM_CLOSE:
		{
			PostQuitMessage(0);
		}
		break;
		default:
		{
			result = DefWindowProc(window, message, wparam, lparam);
		}
		break;
	}
	return result;
}

struct SysWindow *
sys_create_window(u32 width, u32 height)
{
	SysWindow *window = calloc(1, sizeof(SysWindow));
	
	HINSTANCE instance = GetModuleHandle(0);
	
	WNDCLASSA window_class = {0};
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = win32_main_window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = "__window__";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	
	RegisterClassA(&window_class);
	
	window->hwnd = CreateWindowExA(WS_EX_APPWINDOW,
								   window_class.lpszClassName,
								   "ray",
								   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								   CW_USEDEFAULT, CW_USEDEFAULT,
								   width, height,
								   0, 0,
								   instance,
								   0);
	
	ShowWindow(window->hwnd, SW_SHOW);
	UpdateWindow(window->hwnd);
	
	return window;
}

static void 
win32_opengl_set_pixel_format(HDC hdc, SysGLCTX *ctx)
{
	int suggested_pixel_format_index = 0;
	u32 extended_pick = 0;
	
	if (ctx->wglChoosePixelFormatARB)
	{
		const int attributes[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_ALPHA_BITS_ARB, 8,
			WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, 4,
			0
		};
		ctx->wglChoosePixelFormatARB(hdc, attributes, 0, 1, 
									 &suggested_pixel_format_index, &extended_pick);
	}
	
	if (!extended_pick)
	{
		PIXELFORMATDESCRIPTOR desired_pixel_format = { 0 };
		desired_pixel_format.nSize = sizeof(desired_pixel_format);
		desired_pixel_format.nVersion = 1;
		desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
		desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
		desired_pixel_format.cColorBits = 32;
		desired_pixel_format.cAlphaBits = 8;
		desired_pixel_format.cDepthBits = 24;
		desired_pixel_format.iLayerType = PFD_MAIN_PLANE;
		
		suggested_pixel_format_index = ChoosePixelFormat(hdc, &desired_pixel_format);
	}
	
	PIXELFORMATDESCRIPTOR suggested_pixel_format;
	DescribePixelFormat(hdc, suggested_pixel_format_index,
						sizeof(suggested_pixel_format), &suggested_pixel_format);
	SetPixelFormat(hdc, suggested_pixel_format_index, &suggested_pixel_format);
}

SysGLCTX *
sys_init_opengl(SysWindow *window, u32 swap_interval)
{
	SysGLCTX *ctx = calloc(1, sizeof(SysGLCTX));
	
	// Load plain wgl procusrues
	
	HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
	assert(opengl_dll);
	*(FARPROC *)&ctx->wglCreateContext = GetProcAddress(opengl_dll, "wglCreateContext");
	*(FARPROC *)&ctx->wglDeleteContext = GetProcAddress(opengl_dll, "wglDeleteContext");
	*(FARPROC *)&ctx->wglGetProcAddress = GetProcAddress(opengl_dll, "wglGetProcAddress");
	*(FARPROC *)&ctx->wglGetCurrentDC = GetProcAddress(opengl_dll, "wglGetCurrentDC");
	*(FARPROC *)&ctx->wglMakeCurrent = GetProcAddress(opengl_dll, "wglMakeCurrent");
	*(FARPROC *)&ctx->wglSwapLayerBuffers = GetProcAddress(opengl_dll, "wglSwapLayerBuffers");
	// Load wgl extension procedures
	WNDCLASSA dummy_window_class = {0};
	dummy_window_class.lpfnWndProc = DefWindowProcA;
	dummy_window_class.hInstance = GetModuleHandle(0);
	dummy_window_class.lpszClassName = "__dummy__";
	RegisterClassA(&dummy_window_class);
	HWND dummy_hwnd = CreateWindowA(dummy_window_class.lpszClassName, "", 0,
									CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									0, 0, dummy_window_class.hInstance, 0);
	HDC dummy_window_dc = GetDC(dummy_hwnd);
	win32_opengl_set_pixel_format(dummy_window_dc, ctx);
	HGLRC oglrc = ctx->wglCreateContext(dummy_window_dc);
	ctx->wglMakeCurrent(dummy_window_dc, oglrc);
	
	*(FARPROC *)&ctx->wglChoosePixelFormatARB = ctx->wglGetProcAddress("wglChoosePixelFormatARB");
    *(FARPROC *)&ctx->wglCreateContextAttribsARB = ctx->wglGetProcAddress("wglCreateContextAttribsARB");
    *(FARPROC *)&ctx->wglMakeContextCurrentARB = ctx->wglGetProcAddress("wglMakeContextCurrentARB");
    *(FARPROC *)&ctx->wglSwapIntervalEXT = ctx->wglGetProcAddress("wglSwapIntervalEXT");
    *(FARPROC *)&ctx->wglGetSwapIntervalEXT = ctx->wglGetProcAddress("wglGetSwapIntervalEXT");
	ctx->wglDeleteContext(oglrc);
	
	ReleaseDC(dummy_hwnd, dummy_window_dc);
	DestroyWindow(dummy_hwnd);
	
	HDC main_window_dc = GetDC(window->hwnd);
    win32_opengl_set_pixel_format(main_window_dc, ctx);
    const int opengl_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_FLAGS_ARB, 
		WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
	    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	    0 // !!!
    };
	
    HGLRC opengl_rc = ctx->wglCreateContextAttribsARB(main_window_dc, 0, opengl_attributes);
    ctx->wglMakeCurrent(main_window_dc, opengl_rc);
	
	ctx->opengl_dll = opengl_dll;
	
	return ctx;
}

struct OpenGLRenderer *
sys_create_renderer(struct SysGLCTX *ctx)
{
	OpenGLRenderer *renderer = calloc(1, sizeof(OpenGLRenderer));
	
#define GLProc(name, type) \
*(PROC *)&renderer->gl. name = ctx->wglGetProcAddress(#name); \
if (!renderer->gl. name) *(FARPROC *)&renderer->gl. name = GetProcAddress(ctx->opengl_dll, #name); 
#include "editor/renderer/gl_proc_list.inc"
#undef GLProc
	
	return renderer;
}

static Key
win32_vk_to_key(DWORD vk_key)
{
	Key result = Key_Unknown;
	switch (vk_key)
	{
		
#define Win32Keycode(a, b) \
case a:                \
result = b;        \
break;
		
		Win32Keycode(0x00B, Key_0)
			Win32Keycode(0x002, Key_1)
			Win32Keycode(0x003, Key_2)
			Win32Keycode(0x004, Key_3)
			Win32Keycode(0x005, Key_4)
			Win32Keycode(0x006, Key_5)
			Win32Keycode(0x007, Key_6)
			Win32Keycode(0x008, Key_7)
			Win32Keycode(0x009, Key_8)
			Win32Keycode(0x00A, Key_9)
			Win32Keycode(0x01E, Key_A)
			Win32Keycode(0x030, Key_B)
			Win32Keycode(0x02E, Key_C)
			Win32Keycode(0x020, Key_D)
			Win32Keycode(0x012, Key_E)
			Win32Keycode(0x021, Key_F)
			Win32Keycode(0x022, Key_G)
			Win32Keycode(0x023, Key_H)
			Win32Keycode(0x017, Key_I)
			Win32Keycode(0x024, Key_J)
			Win32Keycode(0x025, Key_K)
			Win32Keycode(0x026, Key_L)
			Win32Keycode(0x032, Key_M)
			Win32Keycode(0x031, Key_N)
			Win32Keycode(0x018, Key_O)
			Win32Keycode(0x019, Key_P)
			Win32Keycode(0x010, Key_Q)
			Win32Keycode(0x013, Key_R)
			Win32Keycode(0x01F, Key_S)
			Win32Keycode(0x014, Key_T)
			Win32Keycode(0x016, Key_U)
			Win32Keycode(0x02F, Key_V)
			Win32Keycode(0x011, Key_W)
			Win32Keycode(0x02D, Key_X)
			Win32Keycode(0x015, Key_Y)
			Win32Keycode(0x02C, Key_Z)
			Win32Keycode(0x028, Key_Apostrophe)
			Win32Keycode(0x02B, Key_Backslash)
			Win32Keycode(0x033, Key_Comma)
			Win32Keycode(0x00D, Key_Equal)
			Win32Keycode(0x029, Key_GraveAccent)
			Win32Keycode(0x01A, Key_LeftBracket)
			Win32Keycode(0x00C, Key_Minus)
			Win32Keycode(0x034, Key_Period)
			Win32Keycode(0x01B, Key_RightBracket)
			Win32Keycode(0x027, Key_Semicolon)
			Win32Keycode(0x035, Key_Slash)
			Win32Keycode(0x056, Key_World_2)
			Win32Keycode(0x00E, Key_Backspace)
			Win32Keycode(0x153, Key_Delete)
			Win32Keycode(0x14F, Key_End)
			Win32Keycode(0x01C, Key_Enter)
			Win32Keycode(0x001, Key_Escape)
			Win32Keycode(0x147, Key_Home)
			Win32Keycode(0x152, Key_Insert)
			Win32Keycode(0x15D, Key_Menu)
			Win32Keycode(0x151, Key_PageDown)
			Win32Keycode(0x149, Key_PageUp)
			Win32Keycode(0x146, Key_Pause)
			Win32Keycode(0x039, Key_Space)
			Win32Keycode(0x00F, Key_Tab)
			Win32Keycode(0x03A, Key_CapsLock)
			Win32Keycode(0x145, Key_NumLock)
			Win32Keycode(0x046, Key_ScrollLock)
			Win32Keycode(0x03B, Key_F1)
			Win32Keycode(0x03C, Key_F2)
			Win32Keycode(0x03D, Key_F3)
			Win32Keycode(0x03E, Key_F4)
			Win32Keycode(0x03F, Key_F5)
			Win32Keycode(0x040, Key_F6)
			Win32Keycode(0x041, Key_F7)
			Win32Keycode(0x042, Key_F8)
			Win32Keycode(0x043, Key_F9)
			Win32Keycode(0x044, Key_F10)
			Win32Keycode(0x057, Key_F11)
			Win32Keycode(0x058, Key_F12)
			Win32Keycode(0x064, Key_F13)
			Win32Keycode(0x065, Key_F14)
			Win32Keycode(0x066, Key_F15)
			Win32Keycode(0x067, Key_F16)
			Win32Keycode(0x068, Key_F17)
			Win32Keycode(0x069, Key_F18)
			Win32Keycode(0x06A, Key_F19)
			Win32Keycode(0x06B, Key_F20)
			Win32Keycode(0x06C, Key_F21)
			Win32Keycode(0x06D, Key_F22)
			Win32Keycode(0x06E, Key_F23)
			Win32Keycode(0x076, Key_F24)
			Win32Keycode(0x038, Key_LeftAlt)
			Win32Keycode(0x01D, Key_LeftControl)
			Win32Keycode(0x02A, Key_LeftShift)
			Win32Keycode(0x15B, Key_LeftSuper)
			Win32Keycode(0x137, Key_PrintScreen)
			Win32Keycode(0x138, Key_RightAlt)
			Win32Keycode(0x11D, Key_RightControl)
			Win32Keycode(0x036, Key_RightShift)
			Win32Keycode(0x15C, Key_RightSuper)
			Win32Keycode(0x150, Key_Down)
			Win32Keycode(0x14B, Key_Left)
			Win32Keycode(0x14D, Key_Right)
			Win32Keycode(0x148, Key_Up)
			Win32Keycode(0x052, Key_KP_0)
			Win32Keycode(0x04F, Key_KP_1)
			Win32Keycode(0x050, Key_KP_2)
			Win32Keycode(0x051, Key_KP_3)
			Win32Keycode(0x04B, Key_KP_4)
			Win32Keycode(0x04C, Key_KP_5)
			Win32Keycode(0x04D, Key_KP_6)
			Win32Keycode(0x047, Key_KP_7)
			Win32Keycode(0x048, Key_KP_8)
			Win32Keycode(0x049, Key_KP_9)
			Win32Keycode(0x04E, Key_KP_Add)
			Win32Keycode(0x053, Key_KP_Decimal)
			Win32Keycode(0x135, Key_KP_Divide)
			Win32Keycode(0x11C, Key_KP_Enter)
			Win32Keycode(0x059, Key_KP_Equal)
			Win32Keycode(0x037, Key_KP_Multiply)
			Win32Keycode(0x04A, Key_KP_Subtract)
	}
	return result;
}

void sys_update_input(struct SysWindow *window, Input *input)
{
	input->mouse_wheel = 0;
	
	// Poll events - update keyboard and mouse wheel
	MSG message;
	while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) // @NOTE Get messages from current thread
	{
		WPARAM wparam = message.wParam;
		LPARAM lparam = message.lParam;
		switch (message.message)
		{
			default:
			{
				TranslateMessage(&message);
				DispatchMessageA(&message);
			}
			break;
			case WM_MOUSEWHEEL:
			{
				i16 wheel_delta = HIWORD(wparam);
				input->mouse_wheel += (f32)wheel_delta / WHEEL_DELTA;
			}
			break;
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			{
				//u32 vk_code = wparam;
#define WIN32_KEY_DOWN_BIT (1 << 31)
				bool is_down = ((lparam & WIN32_KEY_DOWN_BIT) == 0);
				i32 scancode = (HIWORD(lparam) & (KF_EXTENDED | 0xFF));
				
				if (!scancode)
				{
					scancode = MapVirtualKeyW((UINT)wparam, MAPVK_VK_TO_VSC);
				}
				
				Key key = win32_vk_to_key(scancode);
				
				assert(key);
				update_key_state(&input->keys[key], is_down);
			}
			break;
			case WM_DESTROY:
			case WM_CLOSE:
			case WM_QUIT:
			{
				input->is_quit_requested = true;
			}
			break;
		}
	}
	
	// Update mouse
	const DWORD win32_mouse_buttons[] = {
		VK_LBUTTON,
		VK_MBUTTON,
		VK_RBUTTON,
		VK_XBUTTON1,
		VK_XBUTTON1};
	
	for (u32 mouse_button_index = 0;
		 mouse_button_index < MOUSE_BUTTON_COUNT;
		 ++mouse_button_index)
	{
		DWORD vk = win32_mouse_buttons[mouse_button_index];
		bool is_down = GetKeyState(vk) & WIN32_KEY_DOWN_BIT;
		update_key_state(&input->keys[FIRST_MOUSE_BUTTON + mouse_button_index], is_down);
	}
	
	POINT win32_mouse;
	GetCursorPos(&win32_mouse);
	ScreenToClient(window->hwnd, &win32_mouse);
	Vec2 mouse = vec2(win32_mouse.x, win32_mouse.y);
	
	input->mouse_delta_x = mouse.x - input->mouse_x;
	input->mouse_delta_y = mouse.y - input->mouse_y;
	input->mouse_x = mouse.x;
	input->mouse_y = mouse.y;
	
	RECT client_rect;
	GetClientRect(window->hwnd, &client_rect);
	Vec2 window_size = vec2(client_rect.right - client_rect.left,
							client_rect.bottom - client_rect.top);
	input->window_size = window_size;
}

void 
sys_swap_buffers(struct SysWindow *window, struct SysGLCTX *gl, u32 swap_interval)
{
    u32 current_swap_interval = gl->wglGetSwapIntervalEXT();
    if (swap_interval != current_swap_interval)
    {
        gl->wglSwapIntervalEXT(swap_interval);
    }
	
    gl->wglSwapLayerBuffers(gl->wglGetCurrentDC(), WGL_SWAP_MAIN_PLANE);
}
