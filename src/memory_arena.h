#if !defined(MEMORY_ARENA_H)

#include "general.h"

#define DEFAULT_ALIGNMENT (2 * sizeof(void *))

typedef struct {
    u8 *data;
    u64 last_data_size;
    u64 data_size;
    u64 data_capacity;
    
    u64 peak_size;
    u32 temp_count;
} MemoryArena;

typedef struct {
    MemoryArena *arena;
    u64 data_size;
    u64 last_data_size;
} TempMemory;

inline TempMemory 
temp_memory_begin(MemoryArena *arena) {
    ++arena->temp_count;
    return (TempMemory) {
        .arena = arena,
        .data_size = arena->data_size,
        .last_data_size = arena->last_data_size
    };
}

inline void
temp_memory_end(TempMemory mem) {
    assert(mem.arena->temp_count);
    --mem.arena->temp_count;
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
    
    if (size) {
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
    }
    
    if (a->data_size > a->peak_size) {
        a->peak_size = a->data_size;
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

inline void *
arena_copy(MemoryArena *a, void *src, u64 size) {
    void *result = arena_alloc(a, size);
    memcpy(result, src, size);
    return result;
}

#define MEMORY_ARENA_H 1
#endif
