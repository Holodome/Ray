#if !defined(MEMORY_POOL_H)

#include "common.h"

typedef struct {
	u64 size;
	u64 used;

	u8 *base;

	bool is_dynamically_growing;
	u64  minimum_block_size;

	i32 temp_count;
} MemoryArena;

typedef struct {
	u64         used;
	MemoryArena *pool;
} TemporaryMemory;

TemporaryMemory temporary_memory_begin(MemoryArena *pool);
void            temporary_memory_end(TemporaryMemory temporary_memory);

typedef struct {
	bool not_clear_to_zero;
	u32  alignment;
} MemoryArenaAllocationParams;

const MemoryArenaAllocationParams default_allocation_params = { false, 8 };

// Creates memory pool from given memory, not growing
MemoryArena memory_arena(void *storage, u64 storage_size);
// Creates growing memory pool
#define DEFAULT_BLOCK_SIZE megabytes(1)
MemoryArena memory_arena_dyn(u64 minimum_block_size);

void memory_arena_clear(MemoryArena *pool);

#define memory_arena_alloc_struct(pool, T, ...) (T *)memory_arena_alloc(pool, sizeof(T), (default_allocation_params, ##__VA_ARGS__))
#define memory_arena_alloc_array(pool, T, n, ...) (T *)memory_arena_alloc(pool, sizeof(T) * n, (default_allocation_params, ##__VA_ARGS__))
void *memory_arena_alloc(MemoryArena *pool, u64 size, MemoryArenaAllocationParams params);

#define MEMORY_POOL_H 1
#endif
