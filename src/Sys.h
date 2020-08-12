#if !defined(SYS_H)

#include "Common.h"

// @NOTE(hl): Detects target OS.
// Most functions from linux and macos we use are part posix, so we can unify them

#define OS_WINDOWS 0
#define OS_POSIX   0

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#undef  OS_WINDOWS
    #define OS_WINDOWS 1
#elif defined(__APPLE__)
	#include <TargetConditionals.h>
    _Static_assert(TARGET_OS_MAC, "Only MacOS build on apple platforms is supported");
    
    #define OS_POSIX 1
#elif defined(__linux__)
    #define OS_POSIX 1
#elif defined(__unix__)
    #define OS_POSIX 1
#elif defined(_POSIX_VERSION)
    #define OS_POSIX 1
#else
	#error "Unknown OS"
#endif

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

typedef struct 
{
    u32 id;  
} Thread;

u32    sys_get_processor_count(void);
Thread sys_create_thread(ThreadProc *proc, void *param);
// @NOTE(hl): Needs to be inlined in case compiler does something stupid.
// Exits current thread
__attribute__((always_inline)) void sys_exit_thread(void);

// Interlocked add. Returns contents of value before addition
// @NOTE(hl): Although this is not os function, but x64 instruction wrapper, we put in sys
inline u64 atomic_add64(volatile u64 *value, u64 addend);

#define SYS_H 1
#endif
