#include "allocator.h"

size_t oaf_align_forward(size_t value, size_t alignment)
{
    if (alignment == 0)
    {
        return value;
    }

    size_t mask = alignment - 1;
    return (value + mask) & ~mask;
}

void* oaf_allocator_alloc(OafAllocator* allocator, size_t size, size_t alignment)
{
    if (allocator == NULL || allocator->ops.alloc == NULL)
    {
        return NULL;
    }

    return allocator->ops.alloc(allocator->state, size, alignment);
}

void* oaf_allocator_realloc(OafAllocator* allocator, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
    if (allocator == NULL || allocator->ops.realloc == NULL)
    {
        return NULL;
    }

    return allocator->ops.realloc(allocator->state, ptr, old_size, new_size, alignment);
}

void oaf_allocator_free(OafAllocator* allocator, void* ptr)
{
    if (allocator == NULL || allocator->ops.free == NULL)
    {
        return;
    }

    allocator->ops.free(allocator->state, ptr);
}
