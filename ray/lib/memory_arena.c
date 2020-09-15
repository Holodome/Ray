#include "memory_arena.h"


TemporaryMemory
temporary_memory_begin(MemoryArena *pool)
{
	TemporaryMemory result = {0};
	result.pool = pool;
	result.used = pool->used;

	++pool->temp_count;
	return result;
}

void
temporary_memory_end(TemporaryMemory temporary_memory)
{
	temporary_memory.pool->used = temporary_memory.used;
	--temporary_memory.pool->temp_count;
}

MemoryArena
memory_arena(void *storage, u64 storage_size)
{
	MemoryArena result = {0};
	result.base = (u8 *)storage;
	result.size = storage_size;
	return result;
}

MemoryArena
memory_arena_dyn(u64 minimum_block_size)
{
	MemoryArena result = {0};
	result.is_dynamically_growing = true;
	result.minimum_block_size     = minimum_block_size;
	return result;
}

void
memory_arena_clear(MemoryArena *pool)
{
	assert(!pool->temp_count);
	assert(!pool->is_dynamically_growing);
	pool->used = 0;
}

static u64
memory_arena_get_alignment_offset(MemoryArena *pool, u64 alignment)
{
	assert(is_power_of_two(alignment));
	u64 alignment_mask = alignment - 1;

	u64 alignment_offset = 0;

	umm result_pointer = (umm)pool->base + pool->used;
	if (result_pointer & alignment_mask)
	{
		alignment_offset = alignment - (result_pointer & alignment_mask);
	}

	return alignment_offset;
}


static u64
memory_arena_get_effective_size_for(MemoryArena *pool, u64 size_requested, MemoryArenaAllocationParams params)
{
	u64 size = size_requested;
	u64 alignemnt_offset = memory_arena_get_alignment_offset(pool, params.alignment);

	size += alignemnt_offset;
	return size;
}

void *
memory_arena_alloc(MemoryArena *pool, u64 size_requested, MemoryArenaAllocationParams params)
{
	void *result = 0;

	u64 size = memory_arena_get_effective_size_for(pool, size_requested, params);
	if ((pool->used + size) > pool->size)
	{
		if (pool->is_dynamically_growing)
		{
			assert(pool->minimum_block_size);

			size = size_requested;
			u64 alloc_size = max(size, pool->minimum_block_size);

			pool->size = alloc_size;
			pool->used = 0;
			pool->base = (u8 *)malloc(alloc_size);
		}
		else
		{
			printf("Memory pool size exceeded! Cap is %llu, current %llu, requested %llu\n", pool->size, pool->used, size);
		}
	}

	assert((pool->used + size_requested) <= pool->size);

	u64 alignment_offset = memory_arena_get_alignment_offset(pool, params.alignment);
	result = pool->base + pool->used + alignment_offset;
	pool->used += size;

	assert(size >= size_requested);

	if (!params.not_clear_to_zero)
	{
		memset(result, 0, size_requested);
	}

	return result;
}
