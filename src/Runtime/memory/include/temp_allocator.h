#ifndef OAF_TEMP_ALLOCATOR_H
#define OAF_TEMP_ALLOCATOR_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_TEMP_MAX_MARKS 128

typedef struct OafTempAllocatorState
{
    unsigned char* buffer;
    size_t capacity;
    size_t offset;
    size_t marks[OAF_TEMP_MAX_MARKS];
    size_t mark_count;
} OafTempAllocatorState;

int oaf_temp_allocator_init(OafTempAllocatorState* state, size_t capacity);
void oaf_temp_allocator_destroy(OafTempAllocatorState* state);
size_t oaf_temp_allocator_mark(OafTempAllocatorState* state);
int oaf_temp_allocator_reset_to_mark(OafTempAllocatorState* state, size_t mark_index);
void oaf_temp_allocator_clear(OafTempAllocatorState* state);
void oaf_temp_allocator_as_allocator(OafTempAllocatorState* state, OafAllocator* allocator);

#ifdef __cplusplus
}
#endif

#endif
