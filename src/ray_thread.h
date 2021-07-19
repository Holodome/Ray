#if !defined(RAY_THREAD_H)

#include "general.h"

#if OS_WINDOWS
#define THREAD_PROC_SIGNATURE(_name) unsigned long _name(void *param)
#elif OS_LINUX || OS_MACOS
#define THREAD_PROC_SIGNATURE(_name) void *_name(void *param)
#else 
#error !
#endif 
typedef THREAD_PROC_SIGNATURE(ThreadProc);

typedef struct {
    u64 id;
} Thread;

Thread create_thread(ThreadProc *proc, void *param);
void exit_thread(void);
u32 get_core_count(void);

static inline u32 get_thread_id(void);

static inline u64 atomic_add64(volatile u64 *value, u64 addend);

#define RAY_THREAD_H 1
#endif
