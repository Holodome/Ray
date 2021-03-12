#if !defined(RAY_MATH_INTRINSICS_H)

#include <general.h>

#if COMPILER_CLANG
// @NOTE: this is strange error, just ignore it for now
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstatic-in-static inline"
#include <x86intrin.h>
#elif COMPILER_MSVC 
#include <xmmintrin.h>
#endif 

#define USE_MATH_H 0

static inline f32 
sqrt32(f32 a) {
#if USE_MATH_H
    return sqrtf(a);
#else 
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a)));
#endif 
}

static inline f32 
rsqrt32(f32 a) {
#if USE_MATH_H
    return 1.0f / sqrtf(a);
#else 
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(a)));
#endif 
}

static inline f32 
max32(f32 a, f32 b) {
#if USE_MATH_H
    return fmaxf(a, b);
#else 
    return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif 
}

static inline f32 
min32(f32 a, f32 b) {
#if USE_MATH_H
    return fminf(a, b);
#else 
    return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a), _mm_set_ss(b)));
#endif 
}

static inline f32 
abs32(f32 a) {
#if USE_MATH_H
    return fabsf(a);
#else 
    u32 temp = *(u32 *)&a & 0x7FFFFFFF;
    return *(f32 *)&temp;
#endif 
}

#if COMPILER_CLANG
#pragma clang diagnostic pop
#endif 

#define RAY_MATH_INTRINSICS_H 1
#endif
