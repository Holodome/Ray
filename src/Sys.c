#include "Sys.h"

#if OS_WINDOWS

#include <windows.h>
#include <intrin.h>

u32
sys_get_processor_count(void)
{
    SYSTEM_INFO info = { 0 };
    GetSystemInfo(&info);
    u32 result = info.dwNumberOfProcessors;
    return result;
}

Thread 
sys_create_thread(ThreadProc *proc, void *param)
{
    Thread result = { 0 };
    
    DWORD thread_id;
    HANDLE thread_handle = CreateThread(0, 0, proc, param, 0, &thread_id);
    CloseHandle(thread_handle);
    
    result.id = thread_id;
    
    return result;
}

void 
sys_exit_thread(void)
{
    ExitThread(0);    
}

inline u64 
atomic_add64(volatile u64 *value, u64 addend)
{
    u64 result = _InterlockedExchangeAdd64((volatile long long *)value, addend);
    return result;
}

#elif OS_POSIX

#include <pthread.h>

Thread 
sys_create_thread(ThreadProc *proc, void *param)
{
    Thread result = { 0 };
    
    // @TODO(hl): Check what pthread_t is 
    pthread_create(&result.id, 0, proc, param);   
    
    return result;
}

void 
sys_exit_thread(void)
{
    pthread_exit(NULL);
}

#else 
#error "!"
#endif 