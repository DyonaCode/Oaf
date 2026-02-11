#include <stdlib.h>
#include "default_allocator.h"
#include "leak_detector.h"

static void* default_alloc(void* state_ptr, size_t size, size_t alignment)
{
    (void)alignment;

    OafDefaultAllocatorState* state = (OafDefaultAllocatorState*)state_ptr;
    void* ptr = malloc(size == 0 ? 1 : size);

    if (ptr == NULL)
    {
        state->failed_allocations++;
        return NULL;
    }

    state->active_allocations++;
    state->total_allocated_bytes += size;
    if (state->leak_detector != NULL)
    {
        oaf_leak_detector_track_alloc(state->leak_detector, ptr, size);
    }
    return ptr;
}

static void* default_realloc(void* state_ptr, void* ptr, size_t old_size, size_t new_size, size_t alignment)
{
    (void)alignment;

    OafDefaultAllocatorState* state = (OafDefaultAllocatorState*)state_ptr;
    void* resized = realloc(ptr, new_size == 0 ? 1 : new_size);

    if (resized == NULL)
    {
        state->failed_allocations++;
        return NULL;
    }

    if (new_size > old_size)
    {
        state->total_allocated_bytes += (new_size - old_size);
    }

    if (state->leak_detector != NULL)
    {
        if (ptr != NULL)
        {
            oaf_leak_detector_track_free(state->leak_detector, ptr);
        }
        oaf_leak_detector_track_alloc(state->leak_detector, resized, new_size);
    }

    return resized;
}

static void default_free(void* state_ptr, void* ptr)
{
    OafDefaultAllocatorState* state = (OafDefaultAllocatorState*)state_ptr;

    if (ptr != NULL && state->active_allocations > 0)
    {
        state->active_allocations--;
    }

    if (state->leak_detector != NULL && ptr != NULL)
    {
        oaf_leak_detector_track_free(state->leak_detector, ptr);
    }

    free(ptr);
}

void oaf_default_allocator_init(OafDefaultAllocatorState* state, OafAllocator* allocator)
{
    state->active_allocations = 0;
    state->total_allocated_bytes = 0;
    state->failed_allocations = 0;
    state->leak_detector = NULL;

    allocator->state = state;
    allocator->ops.alloc = default_alloc;
    allocator->ops.realloc = default_realloc;
    allocator->ops.free = default_free;
}

void oaf_default_allocator_attach_leak_detector(OafDefaultAllocatorState* state, OafLeakDetector* detector)
{
    if (state == NULL)
    {
        return;
    }

    state->leak_detector = detector;
}
