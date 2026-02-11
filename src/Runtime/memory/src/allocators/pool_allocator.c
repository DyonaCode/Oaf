#include <stdlib.h>
#include "pool_allocator.h"

static void* pool_alloc(void* state_ptr, size_t size, size_t alignment)
{
    (void)alignment;

    OafPoolAllocatorState* state = (OafPoolAllocatorState*)state_ptr;
    if (size > state->block_size || state->free_list == NULL)
    {
        return NULL;
    }

    void* block = state->free_list;
    state->free_list = *(void**)state->free_list;
    state->active_blocks++;
    return block;
}

static void* pool_realloc(void* state_ptr, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void)old_size;
    (void)alignment;

    OafPoolAllocatorState* state = (OafPoolAllocatorState*)state_ptr;
    if (ptr == NULL)
    {
        return pool_alloc(state_ptr, new_size, alignment);
    }

    if (new_size <= state->block_size)
    {
        return ptr;
    }

    return NULL;
}

static void pool_free(void* state_ptr, void* ptr)
{
    OafPoolAllocatorState* state = (OafPoolAllocatorState*)state_ptr;

    if (ptr == NULL)
    {
        return;
    }

    *(void**)ptr = state->free_list;
    state->free_list = ptr;

    if (state->active_blocks > 0)
    {
        state->active_blocks--;
    }
}

int oaf_pool_allocator_init(OafPoolAllocatorState* state, size_t block_size, size_t block_count)
{
    if (block_size < sizeof(void*) || block_count == 0)
    {
        return 0;
    }

    state->buffer = (unsigned char*)malloc(block_size * block_count);
    if (state->buffer == NULL)
    {
        state->free_list = NULL;
        state->block_size = 0;
        state->block_count = 0;
        state->active_blocks = 0;
        return 0;
    }

    state->block_size = block_size;
    state->block_count = block_count;
    state->active_blocks = 0;
    state->free_list = NULL;

    for (size_t i = 0; i < block_count; i++)
    {
        void* block = state->buffer + (i * block_size);
        *(void**)block = state->free_list;
        state->free_list = block;
    }

    return 1;
}

void oaf_pool_allocator_destroy(OafPoolAllocatorState* state)
{
    free(state->buffer);
    state->buffer = NULL;
    state->free_list = NULL;
    state->block_size = 0;
    state->block_count = 0;
    state->active_blocks = 0;
}

void oaf_pool_allocator_as_allocator(OafPoolAllocatorState* state, OafAllocator* allocator)
{
    allocator->state = state;
    allocator->ops.alloc = pool_alloc;
    allocator->ops.realloc = pool_realloc;
    allocator->ops.free = pool_free;
}
