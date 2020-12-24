#if !defined(GENERAL_H)

#if !defined(RAY_INTERNAL)
#define RAY_INTERNAL 1
#endif 

#define COMPILER_MSVC 0 
#define COMPILER_CLANG 0
#define COMPILER_GCC 0 
#if defined(__clang__)
#undef COMPILER_CLANG
#define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#undef COMPILER_MSVC 
#define COMPILER_MSVC 1
#elif defined(__GNUC__)
#undef COMPILER_GCC 
#define COMPILER_GCC 1
#else 
#error "Unsupported compiler"
#endif 

#if COMPILER_MSVC
#define UNREACHABLE __assume(0)
#elif COMPILER_CLANG || COMPILER_GCC
#define UNREACHABLE __builtin_unreachable()
#endif 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <search.h>
#include <string.h>
#include <float.h>
#include <ctype.h>

#if RAY_INTERNAL
#include <assert.h>
#else 
// @NOTE Supresses warnings about unused variables
#define assert(...) ((void)(__VA_ARGS__))
#endif 
#define INVALID_DEFAULT_CASE default: assert(!"Invalid default case"); UNREACHABLE; 

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef uintptr_t umm;

#define I8_MIN  INT8_MIN
#define I16_MIN INT16_MIN
#define I32_MIN INT32_MIN
#define I64_MIN INT64_MIN
#define I8_MAX  INT8_MAX
#define I16_MAX INT16_MAX
#define I32_MAX INT32_MAX
#define I64_MAX INT64_MAX
#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define CT_ASSERT(_expr) static_assert(_expr, #_expr)

#define KILOBYTES(_b) ((u64)(_b) << 10)
#define MEGABYTES(_b) (KILOBYTES(_b) << 10)

#define ARRAY_SIZE(_arr) (sizeof(_arr) / sizeof(*(_arr)))

// App settings
#define ENABLE_RUSSIAN_ROULETTE 1
#define DISTANCE_EPSILON        0.001f

#define GENERAL_H 1
#endif
