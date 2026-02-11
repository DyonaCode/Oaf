#ifndef OAFLANG_POOL_ALLOCATOR_H
#define OAFLANG_POOL_ALLOCATOR_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafPoolAllocatorState
{
    unsigned char* buffer;
    void* free_list;
    size_t block_size;
    size_t block_count;
    size_t active_blocks;
} OafPoolAllocatorState;

int oaf_pool_allocator_init(OafPoolAllocatorState* state, size_t block_size, size_t block_count);
void oaf_pool_allocator_destroy(OafPoolAllocatorState* state);
void oaf_pool_allocator_as_allocator(OafPoolAllocatorState* state, OafAllocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
