//
// Platform-specific interface
//
#if !defined(SYS_H)

#include "common.h"
#include "ray_math.h"

#include "key_list.h"

// @NOTE(hl): Detects target OS.
// Most functions from linux and macos we use are part posix, so we can unify them

#define OS_WINDOWS 0
#define OS_POSIX   0
#define OS_MACOS   0
#define OS_LINUX   0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#undef  OS_WINDOWS
#define OS_WINDOWS 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
static_assert(TARGET_OS_MAC, "Only MacOS build on apple platforms is supported");

#undef  OS_POSIX
#define OS_POSIX 1
#undef  OS_MACOS
#define OS_MACOS 1
#elif defined(__linux__)
#undef  OS_POSIX
#define OS_POSIX 1
#undef  OS_LINUX 
#define OS_LINUX 1
#elif defined(__unix__)
#error "!NS"
#undef  OS_POSIX
#define OS_POSIX 1
#elif defined(_POSIX_VERSION)
#error "!NS"
#undef  OS_POSIX
#define OS_POSIX 1
#else
#error "Unknown OS"
#endif

typedef struct {
    bool is_down;
    u8   transition_count;
} KeyState;

inline void
update_key_state(KeyState *state, bool is_down)
{
	if (state->is_down != is_down)
	{
		state->is_down = is_down;
		++state->transition_count;
	}
}

#define is_key_pressed(key, ...) is_key_pressed_(key, (true, ##__VA_ARGS__))
inline bool
is_key_pressed_(KeyState key, bool repeat)
{
	bool result = key.is_down;
	if (!repeat)
	{
		result = result && (key.transition_count != 0);
	}
	return (result);
}

typedef u8 KeyboardModifier;
enum {
	KeyboardModifier_Alt,
	KeyboardModifier_Control,
	KeyboardModifier_Shift,
	// @NOTE(hl): Windows key
	KeyboardModifier_Super,
	KeyboardModifier_Count
};

// Way of supplying input to application.
// This is not how it is ususally done: SDL uses event system and GLFW uses callbacks.
// By defining struct that contains all separate input events we can easilly obtain all data we actually need
typedef struct {
    union {
        Vec2 mouse_pos;
        struct { f32 mouse_x, mouse_y; };
    };
    union {
        Vec2 mouse_delta;  
        struct { f32 mouse_delta_x, mouse_delta_y; };
    };
    f32 mouse_wheel;
    u32 utf32_input[16];
    KeyState keys[Key_Count];
    bool modifiers[KeyboardModifier_Count];
    
    Vec2 window_size;
    bool has_focus;
    bool is_quit_requested;
    
    f32 dt;
    f32 time;
    clock_t frame_start_time;
} Input;

// 
// Threading 
// 

// @NOTE(hl): Thread proc signature in WINAPI is   DWORD WINAPI (*)(LPVOID param)
// Thread proc signature in POSIX is               void *       (*)(void * param)
#if OS_WINDOWS
#define THREAD_PROC_SIGNATURE(name) unsigned long name(void *param)
#elif OS_POSIX 
#define THREAD_PROC_SIGNATURE(name) void *name(void *param)
#else 
#error "!!!"
#endif 

typedef THREAD_PROC_SIGNATURE(ThreadProc);

typedef struct {
    u32 id;  
} Thread;

u32    sys_get_processor_count(void);
Thread sys_create_thread(ThreadProc *proc, void *param);
// @NOTE(hl): Needs to be inlined in case compiler does something stupid.
// Exits calling thread
__attribute__((always_inline)) void sys_exit_thread(void);

// Interlocked add. Returns contents of value before addition
// @NOTE(hl): Although this is not os function, but x64 instruction wrapper, we put it in sys
inline u64 atomic_add64(volatile u64 *value, u64 addend);

// This is SDL-like graphics and window API

// @NOTE(hl): Defined in platform implementation
// Holds data about window. Typically this is only window handle
struct SysWindow;
// Holds platform-specific opengl data. This may include functions like wgl... on windows
struct SysGLCTX;
// Actual opengl renderer. Defined not in platform, but in opengl.h. This is opengl 
// state throughout the program. Also includes pointers to all opengl procedures.
// @TODO(hl): See if it is better to switch to regular ogl procedure approach, where they are global variables
struct OpenGLRenderer;

// All functions' return values are heap-allocated, because size of each structure is platform-dependent
// Creates window 
struct SysWindow *sys_create_window(u32 width, u32 height);
// Initialized opengl 3.3
struct SysGLCTX  *sys_init_opengl(struct SysWindow *window, u32 swap_interval);
struct OpenGLRenderer *sys_create_renderer(struct SysGLCTX *ctx);
// Polls window events and writes it all in input parameter
void sys_update_input(struct SysWindow *window, Input *input);
// Updates window
void sys_swap_buffers(struct SysWindow *window, struct SysGLCTX *gl, u32 swap_interval);

#define SYS_H 1
#endif
