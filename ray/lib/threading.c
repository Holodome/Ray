#include "lib/threading.h"

#if OS_WINDOWS

#define WIN32_LEAD_AND_MEAN
#include <windows.h>

#include <intrin.h>

u32 
sys_get_processor_count(void)
{
	SYSTEM_INFO info = {};
	GetSystemInfo(&info);
	u32 result = info.dwNumberOfProcessors;
	return result;
}

Thread
sys_create_thread(ThreadProc *proc, void *param)
{
	Thread result = {};
	
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

u64
atomic_add64(volatile u64 *value, u64 addend)
{
	u64 result = InterlockedExchangeAdd64((volatile long long *)value, addend);
	return result;
}

#elif OS_POSIX

#include <unistd.h>
#include <pthread.h>

u32 
sys_get_processor_count(void)
{
	u32 result = sysconf(_SC_NPROCESSORS_ONLN);
	return result;
}

Thread
sys_create_thread(ThreadProc *proc, void *param)
{
	Thread result = {};
	
	ct_assert(sizeof(pthread_t) <= sizeof(result));
	assert(!pthread_create((pthread_t *)&result, 0, proc, param));
	
	return result;
}

void 
sys_exit_thread(void)
{
	pthread_exit(0);
}

u64
atomic_add64(volatile u64 *value, u64 addend)
{
	u64 result = __sync_fetch_and_add(value, addend);
	return result;
}

#else 
#error "!"
#endif 