#include <stdlib.h>
#include <string.h>
#include "arena_allocator.h"

static void* arena_alloc(void* state_ptr, size_t size, size_t alignment)
{
    OafArenaAllocatorState* state = (OafArenaAllocatorState*)state_ptr;
    size_t aligned_offset = oaf_align_forward(state->offset, alignment == 0 ? 1 : alignment);

    if (aligned_offset + size > state->capacity)
    {
        return NULL;
    }

    void* ptr = state->buffer + aligned_offset;
    state->offset = aligned_offset + size;
    return ptr;
}

static void* arena_realloc(void* state_ptr, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
    OafArenaAllocatorState* state = (OafArenaAllocatorState*)state_ptr;

    if (ptr == NULL)
    {
        return arena_alloc(state_ptr, new_size, alignment);
    }

    void* replacement = arena_alloc(state_ptr, new_size, alignment);
    if (replacement == NULL)
    {
        return NULL;
    }

    memcpy(replacement, ptr, old_size < new_size ? old_size : new_size);
    return replacement;
}

static void arena_free(void* state_ptr, void* ptr)
{
    (void)state_ptr;
    (void)ptr;
}

int oaf_arena_allocator_init(OafArenaAllocatorState* state, size_t capacity)
{
    state->buffer = (unsigned char*)malloc(capacity == 0 ? 1 : capacity);
    if (state->buffer == NULL)
    {
        state->capacity = 0;
        state->offset = 0;
        return 0;
    }

    state->capacity = capacity == 0 ? 1 : capacity;
    state->offset = 0;
    return 1;
}

void oaf_arena_allocator_destroy(OafArenaAllocatorState* state)
{
    free(state->buffer);
    state->buffer = NULL;
    state->capacity = 0;
    state->offset = 0;
}

void oaf_arena_allocator_reset(OafArenaAllocatorState* state)
{
    state->offset = 0;
}

void oaf_arena_allocator_as_allocator(OafArenaAllocatorState* state, OafAllocator* allocator)
{
    allocator->state = state;
    allocator->ops.alloc = arena_alloc;
    allocator->ops.realloc = arena_realloc;
    allocator->ops.free = arena_free;
}
