#ifndef OAFLANG_ALLOCATOR_H
#define OAFLANG_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*OafAllocProc)(void* state, size_t size, size_t alignment);
typedef void* (*OafReallocProc)(void* state, void* ptr, size_t old_size, size_t new_size, size_t alignment);
typedef void (*OafFreeProc)(void* state, void* ptr);

typedef struct OafAllocatorOps
{
    OafAllocProc alloc;
    OafReallocProc realloc;
    OafFreeProc free;
} OafAllocatorOps;

typedef struct OafAllocator
{
    void* state;
    OafAllocatorOps ops;
} OafAllocator;

void* oaf_allocator_alloc(OafAllocator* allocator, size_t size, size_t alignment);
void* oaf_allocator_realloc(OafAllocator* allocator, void* ptr, size_t old_size, size_t new_size, size_t alignment);
void oaf_allocator_free(OafAllocator* allocator, void* ptr);
size_t oaf_align_forward(size_t value, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif
