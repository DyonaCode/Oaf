#ifndef OAF_ARENA_ALLOCATOR_H
#define OAF_ARENA_ALLOCATOR_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafArenaAllocatorState
{
    unsigned char* buffer;
    size_t capacity;
    size_t offset;
} OafArenaAllocatorState;

int oaf_arena_allocator_init(OafArenaAllocatorState* state, size_t capacity);
void oaf_arena_allocator_destroy(OafArenaAllocatorState* state);
void oaf_arena_allocator_reset(OafArenaAllocatorState* state);
void oaf_arena_allocator_as_allocator(OafArenaAllocatorState* state, OafAllocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
