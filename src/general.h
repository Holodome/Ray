#if !defined(GENERAL_H)

#if !defined(RAY_INTERNAL)
#define RAY_INTERNAL 0
#endif 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <search.h>
#include <string.h>
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
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))

typedef struct {
    u8 *data;
    u64 last_data_size;
    u64 data_size;
    u64 data_capacity;
} MemoryArena;

typedef struct {
    MemoryArena *arena;
    u64 data_size;
    u64 last_data_size;
} TempMemory;

inline TempMemory 
temp_memory_begin(MemoryArena *arena) {
    return (TempMemory) {
        .arena = arena,
        .data_size = arena->data_size,
        .last_data_size = arena->last_data_size
    };
}

inline void
temp_memory_end(TempMemory mem) {
    mem.arena->data_size = mem.data_size;
    mem.arena->last_data_size = mem.last_data_size;
}

inline MemoryArena 
memory_arena(void *buffer, u64 buffer_size) {
    return (MemoryArena) {
        .data = buffer,
        .data_capacity = buffer_size
    };
}

inline umm 
align_forward(umm ptr, u64 align) {
    assert(!(align & (align - 1)));
    
    umm p = ptr;
    umm a = align;
    umm modulo = p & (a - 1);
    
    if (modulo) {
        p += a - modulo;
    }
    return p;
}

#define arena_alloc(_a, _size) arena_alloc_align(_a, _size, DEFAULT_ALIGNMENT)
inline void *
arena_alloc_align(MemoryArena *a, u64 size, u64 align) {
    void *result = 0;
    
    umm curr_ptr = (umm)a->data + a->data_size;
    umm offset = align_forward(curr_ptr, align);
    offset -= (umm)a->data;
    
    if (offset + size < a->data_capacity) {
        u8 *ptr = a->data + offset;
        a->last_data_size = offset;
        a->data_size = offset + size;
        
        result = ptr;
    } else {
        assert(!"Memory is out of bounds");
    }
    
    if (result) {
        memset(result, 0, size);
    }
    
    return result;
}

#define arena_realloc(_a, _old_mem, _old_size, _new_size) arena_realloc_align(_a, _old_mem, _old_size, _new_size, DEFAULT_ALIGNMENT)
inline void *
arena_realloc_align(MemoryArena *a, void *old_mem_v, u64 old_size, u64 new_size, u64 align) {
    void *result = 0;
    
    u8 *old_mem = (u8 *)old_mem_v;
    
    if (!old_mem || !old_size) {
        result = arena_alloc_align(a, new_size, align);
    } else if (a->data <= old_mem && old_mem < a->data + a->data_capacity) {
        if (a->data + a->last_data_size == old_mem) {
            a->data_size = a->last_data_size + new_size;
            if (new_size > old_size) {
                memset(a->data + a->data_size, 0, new_size - old_size);
            }
            result = old_mem;
        } else {
            result = arena_alloc_align(a, new_size, align);
            u64 copy_size = new_size;
            if (old_size < new_size) {
                copy_size = old_size;
            }
            
            memmove(result, old_mem, copy_size);
        }
    } else {
        assert(!"Memory is out of bounds");
    }
    
    return result;
}

#define GENERAL_H 1
#endif
