#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "atomic_ops.h"
#include "oaf_thread_pool.h"
#include "oaf_async.h"
#include "oaf_parallel.h"

typedef struct SumTaskState
{
    OafAtomicI64* accumulator;
    int64_t value;
} SumTaskState;

static void accumulate_task(void* state)
{
    SumTaskState* task = (SumTaskState*)state;
    oaf_atomic_i64_fetch_add(task->accumulator, task->value);
}

static int test_thread_pool(void)
{
    OafThreadPool pool;
    OafAtomicI64 total;
    SumTaskState tasks[100];
    size_t index;
    const OafThreadPoolStats* stats;

    if (!oaf_thread_pool_init(&pool, 4, 64))
    {
        return 0;
    }

    oaf_atomic_i64_init(&total, 0);
    for (index = 0; index < 100; index++)
    {
        tasks[index].accumulator = &total;
        tasks[index].value = (int64_t)(index + 1u);
        if (!oaf_thread_pool_submit(&pool, accumulate_task, &tasks[index]))
        {
            oaf_thread_pool_shutdown(&pool);
            return 0;
        }
    }

    if (!oaf_thread_pool_wait_idle(&pool))
    {
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    stats = oaf_thread_pool_stats(&pool);
    if (stats == NULL || stats->submitted != 100 || stats->completed != 100 || stats->rejected != 0)
    {
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    if (oaf_atomic_i64_load(&total) != 5050)
    {
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    oaf_thread_pool_shutdown(&pool);
    return 1;
}

typedef struct AddAsyncState
{
    int left;
    int right;
    int result;
} AddAsyncState;

static void* add_async(void* state)
{
    AddAsyncState* add = (AddAsyncState*)state;
    add->result = add->left + add->right;
    return &add->result;
}

static int test_async_await(void)
{
    OafThreadPool pool;
    OafFuture future;
    AddAsyncState state;
    void* result_ptr = NULL;

    state.left = 19;
    state.right = 23;
    state.result = 0;

    if (!oaf_thread_pool_init(&pool, 2, 16))
    {
        return 0;
    }

    if (!oaf_async_submit(&pool, add_async, &state, &future))
    {
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    if (!oaf_future_await(&future, &result_ptr))
    {
        oaf_future_destroy(&future);
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    if (result_ptr != &state.result || *(int*)result_ptr != 42)
    {
        oaf_future_destroy(&future);
        oaf_thread_pool_shutdown(&pool);
        return 0;
    }

    oaf_future_destroy(&future);
    oaf_thread_pool_shutdown(&pool);
    return 1;
}

typedef struct FillState
{
    int64_t* values;
} FillState;

typedef struct MapState
{
    int64_t factor;
} MapState;

typedef struct ReduceState
{
    const int64_t* values;
} ReduceState;

static void fill_sequence(size_t index, void* state)
{
    FillState* fill = (FillState*)state;
    fill->values[index] = (int64_t)(index + 1u);
}

static void map_scale(size_t index, const void* input, void* output, void* state)
{
    const int64_t* source = (const int64_t*)input;
    int64_t* destination = (int64_t*)output;
    MapState* map = (MapState*)state;
    (void)index;
    *destination = (*source) * map->factor;
}

static int64_t reduce_sum(size_t index, void* state)
{
    ReduceState* reduce = (ReduceState*)state;
    return reduce->values[index];
}

static int test_parallel_algorithms(void)
{
    OafThreadPool pool;
    int64_t* values;
    int64_t* mapped;
    FillState fill;
    MapState map;
    ReduceState reduce;
    int64_t result;
    const size_t count = 5000;
    const int64_t base_sum = ((int64_t)count * ((int64_t)count + 1)) / 2;
    int ok = 1;

    values = (int64_t*)malloc(sizeof(int64_t) * count);
    mapped = (int64_t*)malloc(sizeof(int64_t) * count);
    if (values == NULL || mapped == NULL)
    {
        free(values);
        free(mapped);
        return 0;
    }

    if (!oaf_thread_pool_init(&pool, 4, 128))
    {
        free(values);
        free(mapped);
        return 0;
    }

    fill.values = values;
    map.factor = 3;
    reduce.values = mapped;

    ok = ok && oaf_parallel_for(&pool, count, 0, fill_sequence, &fill);
    ok = ok && oaf_parallel_map(&pool, values, mapped, count, sizeof(int64_t), 0, map_scale, &map);
    ok = ok && oaf_parallel_reduce_i64(&pool, count, 0, reduce_sum, &reduce, &result);
    ok = ok && (result == (base_sum * map.factor));

    oaf_thread_pool_shutdown(&pool);
    free(values);
    free(mapped);
    return ok;
}

int main(void)
{
    int ok = 1;

    ok = ok && test_thread_pool();
    ok = ok && test_async_await();
    ok = ok && test_parallel_algorithms();

    if (!ok)
    {
        fprintf(stderr, "advanced concurrency smoke tests failed\n");
        return 1;
    }

    printf("advanced concurrency smoke tests passed\n");
    return 0;
}
