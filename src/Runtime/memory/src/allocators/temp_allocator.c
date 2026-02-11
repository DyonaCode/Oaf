#include <stdlib.h>
#include "temp_allocator.h"

static void* temp_alloc(void* state_ptr, size_t size, size_t alignment)
{
    OafTempAllocatorState* state = (OafTempAllocatorState*)state_ptr;
    size_t aligned_offset = oaf_align_forward(state->offset, alignment == 0 ? 1 : alignment);

    if (aligned_offset + size > state->capacity)
    {
        return NULL;
    }

    void* ptr = state->buffer + aligned_offset;
    state->offset = aligned_offset + size;
    return ptr;
}

static void* temp_realloc(void* state_ptr, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void)ptr;
    (void)old_size;
    return temp_alloc(state_ptr, new_size, alignment);
}

static void temp_free(void* state_ptr, void* ptr)
{
    (void)state_ptr;
    (void)ptr;
}

int oaf_temp_allocator_init(OafTempAllocatorState* state, size_t capacity)
{
    state->buffer = (unsigned char*)malloc(capacity == 0 ? 1 : capacity);
    if (state->buffer == NULL)
    {
        state->capacity = 0;
        state->offset = 0;
        state->mark_count = 0;
        return 0;
    }

    state->capacity = capacity == 0 ? 1 : capacity;
    state->offset = 0;
    state->mark_count = 0;
    return 1;
}

void oaf_temp_allocator_destroy(OafTempAllocatorState* state)
{
    free(state->buffer);
    state->buffer = NULL;
    state->capacity = 0;
    state->offset = 0;
    state->mark_count = 0;
}

size_t oaf_temp_allocator_mark(OafTempAllocatorState* state)
{
    if (state->mark_count >= OAF_TEMP_MAX_MARKS)
    {
        return (size_t)-1;
    }

    state->marks[state->mark_count] = state->offset;
    state->mark_count++;
    return state->mark_count - 1;
}

int oaf_temp_allocator_reset_to_mark(OafTempAllocatorState* state, size_t mark_index)
{
    if (mark_index >= state->mark_count)
    {
        return 0;
    }

    state->offset = state->marks[mark_index];
    state->mark_count = mark_index;
    return 1;
}

void oaf_temp_allocator_clear(OafTempAllocatorState* state)
{
    state->offset = 0;
    state->mark_count = 0;
}

void oaf_temp_allocator_as_allocator(OafTempAllocatorState* state, OafAllocator* allocator)
{
    allocator->state = state;
    allocator->ops.alloc = temp_alloc;
    allocator->ops.realloc = temp_realloc;
    allocator->ops.free = temp_free;
}
