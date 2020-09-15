#if !defined(THREADING_H)

#include "lib/common.h"
#include "lib/os.h"

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
void   sys_exit_thread(void);

#define THREADING_H 1
#endif
