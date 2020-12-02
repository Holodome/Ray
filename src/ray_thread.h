#if !defined(RAY_THREAD_H)

#include "general.h"

#define THREAD_PROC_SIGNATURE(_name) unsigned long _name(void *param)
typedef THREAD_PROC_SIGNATURE(ThreadProc);

typedef struct {
    u64 id;
} Thread;

u64 atomic_add64(volatile u64 *value, u64 addend);
Thread create_thread(ThreadProc *proc, void *param);
void exit_thread(void);

#define RAY_THREAD_H 1
#endif
