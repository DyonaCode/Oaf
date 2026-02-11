#include <stdio.h>
#include <string.h>
#include "allocator.h"
#include "default_allocator.h"
#include "arena_allocator.h"
#include "pool_allocator.h"
#include "temp_allocator.h"
#include "ownership.h"
#include "lifetime.h"
#include "bounds.h"
#include "null_safety.h"
#include "leak_detector.h"
#include "gc.h"

static int test_default_allocator(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    oaf_default_allocator_init(&state, &allocator);

    void* ptr = oaf_allocator_alloc(&allocator, 32, 8);
    if (ptr == NULL)
    {
        return 0;
    }

    oaf_allocator_free(&allocator, ptr);
    return state.active_allocations == 0;
}

static int test_arena_allocator(void)
{
    OafArenaAllocatorState state;
    OafAllocator allocator;

    if (!oaf_arena_allocator_init(&state, 128))
    {
        return 0;
    }

    oaf_arena_allocator_as_allocator(&state, &allocator);

    void* first = oaf_allocator_alloc(&allocator, 16, 8);
    void* second = oaf_allocator_alloc(&allocator, 32, 8);
    int ok = first != NULL && second != NULL;

    oaf_arena_allocator_reset(&state);
    void* third = oaf_allocator_alloc(&allocator, 64, 8);
    ok = ok && third != NULL;

    oaf_arena_allocator_destroy(&state);
    return ok;
}

static int test_pool_allocator(void)
{
    OafPoolAllocatorState state;
    OafAllocator allocator;

    if (!oaf_pool_allocator_init(&state, 32, 8))
    {
        return 0;
    }

    oaf_pool_allocator_as_allocator(&state, &allocator);

    void* blocks[8] = {0};
    for (size_t i = 0; i < 8; i++)
    {
        blocks[i] = oaf_allocator_alloc(&allocator, 16, 8);
        if (blocks[i] == NULL)
        {
            oaf_pool_allocator_destroy(&state);
            return 0;
        }
    }

    if (oaf_allocator_alloc(&allocator, 16, 8) != NULL)
    {
        oaf_pool_allocator_destroy(&state);
        return 0;
    }

    oaf_allocator_free(&allocator, blocks[0]);
    if (oaf_allocator_alloc(&allocator, 16, 8) == NULL)
    {
        oaf_pool_allocator_destroy(&state);
        return 0;
    }

    oaf_pool_allocator_destroy(&state);
    return 1;
}

static int test_temp_allocator(void)
{
    OafTempAllocatorState state;
    OafAllocator allocator;

    if (!oaf_temp_allocator_init(&state, 128))
    {
        return 0;
    }

    oaf_temp_allocator_as_allocator(&state, &allocator);

    void* first = oaf_allocator_alloc(&allocator, 24, 8);
    size_t mark = oaf_temp_allocator_mark(&state);
    void* second = oaf_allocator_alloc(&allocator, 24, 8);

    int ok = first != NULL && second != NULL && mark != (size_t)-1;
    ok = ok && oaf_temp_allocator_reset_to_mark(&state, mark);

    void* third = oaf_allocator_alloc(&allocator, 24, 8);
    ok = ok && third != NULL;

    oaf_temp_allocator_destroy(&state);
    return ok;
}

static int test_ownership_and_lifetime(void)
{
    OafOwnershipToken token;
    OafOwnershipToken destination;
    OafLifetimeTracker lifetime;

    oaf_lifetime_init(&lifetime, 16);
    if (!oaf_lifetime_enter_scope(&lifetime))
    {
        return 0;
    }

    oaf_ownership_init(&token, 1, lifetime.current_depth);
    oaf_ownership_init(&destination, 2, lifetime.current_depth);
    oaf_ownership_move(&destination);
    oaf_lifetime_bind_token(&lifetime, &token);

    if (!oaf_ownership_borrow_immutable(&token))
    {
        return 0;
    }

    if (oaf_ownership_move(&token))
    {
        return 0;
    }

    if (!oaf_ownership_release_immutable(&token))
    {
        return 0;
    }

    if (!oaf_ownership_move(&token))
    {
        return 0;
    }

    if (!oaf_ownership_reacquire(&token))
    {
        return 0;
    }

    if (!oaf_ownership_transfer(&token, &destination))
    {
        return 0;
    }

    if (oaf_ownership_can_read(&token))
    {
        return 0;
    }

    if (!oaf_ownership_can_write(&destination))
    {
        return 0;
    }

    if (!oaf_ownership_release(&destination))
    {
        return 0;
    }

    if (oaf_ownership_is_alive(&destination))
    {
        return 0;
    }

    if (oaf_ownership_reacquire(&destination))
    {
        return 0;
    }

    if (!oaf_lifetime_token_is_valid(&lifetime, &token))
    {
        return 0;
    }

    if (oaf_lifetime_token_can_access(&lifetime, &token))
    {
        return 0;
    }

    if (!oaf_lifetime_exit_scope(&lifetime))
    {
        return 0;
    }

    return !oaf_lifetime_token_is_valid(&lifetime, &token)
        && !oaf_lifetime_token_can_access(&lifetime, &token);
}

static int test_bounds_and_null_safety(void)
{
    int values[3] = {10, 20, 30};
    int* value;

    if (!oaf_bounds_check_index(0, 3) || oaf_bounds_check_index(3, 3))
    {
        return 0;
    }

    if (!oaf_bounds_check_range(1, 2, 3) || oaf_bounds_check_range(2, 2, 3))
    {
        return 0;
    }

    value = (int*)oaf_bounds_checked_offset(values, sizeof(int), 1, 3);
    if (value == NULL || *value != 20)
    {
        return 0;
    }

    if (oaf_bounds_checked_offset(values, sizeof(int), 5, 3) != NULL)
    {
        return 0;
    }

    if (oaf_null_is_non_null(NULL))
    {
        return 0;
    }

    if (!oaf_null_is_non_null(values))
    {
        return 0;
    }

    if (oaf_null_require_non_null(NULL) != NULL)
    {
        return 0;
    }

    return oaf_null_require_non_null(values) == values;
}

static int test_leak_detection(void)
{
    OafLeakDetector detector;
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    void* first;
    void* second;

    oaf_leak_detector_init(&detector);
    oaf_default_allocator_init(&state, &allocator);
    oaf_default_allocator_attach_leak_detector(&state, &detector);

    first = oaf_allocator_alloc(&allocator, 32, 8);
    second = oaf_allocator_alloc(&allocator, 64, 8);

    if (first == NULL || second == NULL)
    {
        if (first != NULL)
        {
            oaf_allocator_free(&allocator, first);
        }
        if (second != NULL)
        {
            oaf_allocator_free(&allocator, second);
        }
        return 0;
    }

    oaf_allocator_free(&allocator, first);

    if (!oaf_leak_detector_has_leaks(&detector))
    {
        oaf_allocator_free(&allocator, second);
        return 0;
    }

    if (oaf_leak_detector_active_allocations(&detector) != 1)
    {
        oaf_allocator_free(&allocator, second);
        return 0;
    }

    oaf_allocator_free(&allocator, second);
    return !oaf_leak_detector_has_leaks(&detector)
        && oaf_leak_detector_peak_bytes(&detector) >= 96;
}

static int test_gc_cycle_collection(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafGarbageCollector collector;
    void* first;
    void* second;
    size_t collected;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_gc_init(&collector, &allocator, 1))
    {
        return 0;
    }

    first = oaf_gc_alloc(&collector, 24, 8);
    second = oaf_gc_alloc(&collector, 24, 8);
    if (first == NULL || second == NULL)
    {
        oaf_gc_destroy(&collector);
        return 0;
    }

    if (!oaf_gc_retain(&collector, first) || !oaf_gc_retain(&collector, second))
    {
        oaf_gc_destroy(&collector);
        return 0;
    }

    if (!oaf_gc_add_reference(&collector, first, second)
        || !oaf_gc_add_reference(&collector, second, first))
    {
        oaf_gc_destroy(&collector);
        return 0;
    }

    if (!oaf_gc_detect_cycles(&collector))
    {
        oaf_gc_destroy(&collector);
        return 0;
    }

    oaf_gc_release(&collector, first);
    oaf_gc_release(&collector, second);

    collected = oaf_gc_collect(&collector);
    if (collected != 2 || oaf_gc_object_count(&collector) != 0)
    {
        oaf_gc_destroy(&collector);
        return 0;
    }

    oaf_gc_destroy(&collector);
    return state.active_allocations == 0;
}

int main(void)
{
    int ok = 1;
    ok = ok && test_default_allocator();
    ok = ok && test_arena_allocator();
    ok = ok && test_pool_allocator();
    ok = ok && test_temp_allocator();
    ok = ok && test_ownership_and_lifetime();
    ok = ok && test_bounds_and_null_safety();
    ok = ok && test_leak_detection();
    ok = ok && test_gc_cycle_collection();

    if (!ok)
    {
        fprintf(stderr, "memory smoke tests failed\n");
        return 1;
    }

    printf("memory smoke tests passed\n");
    return 0;
}
