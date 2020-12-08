#if !defined(RAY_THREAD_H)

#include "general.h"


#define THREAD_PROC_SIGNATURE(_name) unsigned long _name(void *param)
typedef THREAD_PROC_SIGNATURE(ThreadProc);

typedef struct {
    u64 id;
} Thread;

Thread create_thread(ThreadProc *proc, void *param);
void exit_thread(void);
u32 get_core_count(void);

inline u32 get_thread_id(void);

inline u64 atomic_add64(volatile u64 *value, u64 addend);

#define RAY_THREAD_H 1
#endif
