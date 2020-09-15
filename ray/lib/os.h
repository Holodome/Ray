#if !defined(OS_H)

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


#define OS_H 1
#endif
