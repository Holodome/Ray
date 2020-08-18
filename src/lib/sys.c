#include "sys.h"

#if OS_WINDOWS

#include "sys_win32.c"

#elif OS_POSIX

#include "sys_posix.c"

#else 
#error "!"
#endif 