// Contains definitaions for things that are used eveywhere
// These are more pleasant to write types and some helper macros
// 
#if !defined(COMMON_H)

// Used in debug builds
#ifndef RAY_INTERNAL 
#define RAY_INTERNAL 1
#endif 

// Detect compiler
// @NOTE(hl): Check for clang first because when compiling clang on windows, it likes to define _MSC_VER as well.
#if defined(__clang__)
#define COMPILER_LLVM 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__GUNC__)
#define COMPILER_GCC 1
#else
#error "Unsupported compiler"
#endif

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif 
#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif 
#ifndef COMPILER_GCC
#define COMPILER_GCC 0
#endif 

// Support for gcc __attribute__ syntax
#if COMPILER_LLVM || COMPILER_GCC
#define ATTRIBUTE(x) __attribute__((x))
#else 
#define ATTRIBUTE(x) 
#endif 

// Ugly pre C11 way to compile-time assert
#define ct_assert(exp) char __a##__LINE__[(exp) ? 1 : -1]

#include <stdbool.h>

#include <stdint.h>
#include <float.h>

#if 0

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t umm;
typedef intptr_t  imm;

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define I8_MAX  INT8_MAX
#define I8_MIN  INT8_MIN
#define I16_MAX INT16_MAX
#define I16_MIN INT16_MIN
#define I32_MAX INT32_MAX
#define I32_MIN INT32_MIN
#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN

#define UMM_MAX UINTPTR_MAX
#define IMM_MAX INTPTR_MAX
#define IMM_MIN INTPTR_MIN

#else 

// @NOTE(hl): x86-64 defines. For some reason stdint.h on WSL is incorrect (WTF? - 64 bit types are defines as long, not long long )
typedef char      i8;
typedef short     i16;
typedef int       i32;
typedef long long i64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

#define U8_MAX  ((u8) 0xFF)
#define U16_MAX ((u16)0xFFFF)
#define U32_MAX ((u32)0xFFFFFFFF)
#define U64_MAX ((u64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX  ((i8) 127)
#define I8_MIN  ((i8)(-127 - 1))
#define I16_MAX ((i16)32767)
#define I16_MIN ((i16)(-32767 - 1))
#define I32_MAX ((i32)2147483647)
#define I32_MIN ((i32)(-2147483647 - 1))
#define I64_MAX ((i64)9223372036854775807)
#define I64_MIN ((i64)(-9223372036854775807 - 1))

typedef u64 umm;
typedef u64 imm;

#define UMM_MAX U64_MAX
#define IMM_MAX I64_MAX
#define IMM_MIN I64_MIN

#endif 

typedef float  f32;
typedef double f64;

#define F32_MAX FLT_MAX
#define F32_MIN FLT_MIN
#define F64_MAX DBL_MAX
#define F64_MIN DBL_MIN

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>

// Evaluates to size of array created on stack. If pointer is being passed instead of array, clang and gcc warn about this.
#define array_size(a) (sizeof(a) / sizeof(*(a)))

// @NOTE(hl): stb_sprintf is way faster than any built-in snprintf function
#include "stb_sprintf.h"

#define format_string(dest, size, ...)               stbsp_snprintf(dest, size, __VA_ARGS__)
#define format_string_list(dest, size, format, args) stbsp_vsnprintf(dest, size, format, args)

#define COMMON_H 1
#endif
