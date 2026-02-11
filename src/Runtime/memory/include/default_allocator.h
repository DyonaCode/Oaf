#ifndef OAF_DEFAULT_ALLOCATOR_H
#define OAF_DEFAULT_ALLOCATOR_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OafLeakDetector;
typedef struct OafLeakDetector OafLeakDetector;

typedef struct OafDefaultAllocatorState
{
    size_t active_allocations;
    size_t total_allocated_bytes;
    size_t failed_allocations;
    OafLeakDetector* leak_detector;
} OafDefaultAllocatorState;

void oaf_default_allocator_init(OafDefaultAllocatorState* state, OafAllocator* allocator);
void oaf_default_allocator_attach_leak_detector(OafDefaultAllocatorState* state, OafLeakDetector* detector);

#ifdef __cplusplus
}
#endif

#endif
