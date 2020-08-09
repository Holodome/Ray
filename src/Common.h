#if !defined(COMMON_H)

#include <stdbool.h>

#include <stdint.h>
#include <float.h>

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
typedef intptr_t  imm;

const u8 U8_MAX   = UINT8_MAX;
const u16 U16_MAX = UINT16_MAX;
const u32 U32_MAX = UINT32_MAX;
const u64 U64_MAX = UINT64_MAX;

const i8 I8_MAX   = INT8_MAX;
const i8 I8_MIN   = INT8_MIN;
const i16 I16_MAX = INT16_MAX;
const i16 I16_MIN = INT16_MIN;
const i32 I32_MAX = INT32_MAX;
const i32 I32_MIN = INT32_MIN;
const i64 I64_MAX = INT64_MAX;
const i64 I64_MIN = INT64_MIN;

const f32 F32_MAX = FLT_MAX;
const f32 F32_MIN = FLT_MIN;
const f64 F64_MAX = DBL_MAX;
const f64 F64_MIN = DBL_MIN;

const umm UMM_MAX = UINTPTR_MAX;
const imm IMM_MAX = INTPTR_MAX;

#define array_size(a) (sizeof(a) / sizeof(*(a)))

#define COMMON_H 1
#endif
