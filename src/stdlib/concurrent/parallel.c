#include <stdlib.h>
#include "oaf_parallel.h"

typedef struct OafParallelLatch
{
    OafMutex mutex;
    OafCondVar completed;
    size_t remaining;
    int failed;
} OafParallelLatch;

typedef struct OafParallelForTask
{
    size_t start;
    size_t end;
    OafParallelForProc proc;
    void* state;
    OafParallelLatch* latch;
} OafParallelForTask;

typedef struct OafParallelMapTask
{
    size_t start;
    size_t end;
    const unsigned char* input;
    unsigned char* output;
    size_t element_size;
    OafParallelMapProc proc;
    void* state;
    OafParallelLatch* latch;
} OafParallelMapTask;

typedef struct OafParallelReduceTaskI64
{
    size_t start;
    size_t end;
    OafParallelReduceProcI64 proc;
    void* state;
    int64_t partial;
    OafParallelLatch* latch;
} OafParallelReduceTaskI64;

static size_t choose_chunk_size(const OafThreadPool* pool, size_t count, size_t preferred)
{
    size_t workers;
    size_t target_tasks;
    size_t chunk;

    if (preferred > 0)
    {
        return preferred;
    }

    workers = oaf_thread_pool_worker_count(pool);
    if (workers == 0)
    {
        workers = 1;
    }

    target_tasks = workers * 4u;
    if (target_tasks == 0)
    {
        target_tasks = 1;
    }

    chunk = count / target_tasks;
    if ((count % target_tasks) != 0)
    {
        chunk++;
    }

    if (chunk == 0)
    {
        chunk = 1;
    }

    return chunk;
}

static size_t task_count_for_range(size_t count, size_t chunk_size)
{
    if (count == 0 || chunk_size == 0)
    {
        return 0;
    }

    return (count + chunk_size - 1u) / chunk_size;
}

static int latch_init(OafParallelLatch* latch, size_t remaining)
{
    if (latch == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_init(&latch->mutex) || !oaf_cond_var_init(&latch->completed))
    {
        oaf_cond_var_destroy(&latch->completed);
        oaf_mutex_destroy(&latch->mutex);
        return 0;
    }

    latch->remaining = remaining;
    latch->failed = 0;
    return 1;
}

static void latch_destroy(OafParallelLatch* latch)
{
    if (latch == NULL)
    {
        return;
    }

    oaf_cond_var_destroy(&latch->completed);
    oaf_mutex_destroy(&latch->mutex);
    latch->remaining = 0;
    latch->failed = 0;
}

static void latch_mark_done(OafParallelLatch* latch, int failed)
{
    if (latch == NULL || !oaf_mutex_lock(&latch->mutex))
    {
        return;
    }

    if (failed)
    {
        latch->failed = 1;
    }

    if (latch->remaining > 0)
    {
        latch->remaining--;
    }

    if (latch->remaining == 0)
    {
        oaf_cond_var_broadcast(&latch->completed);
    }

    oaf_mutex_unlock(&latch->mutex);
}

static void latch_mark_unscheduled(OafParallelLatch* latch, size_t unscheduled)
{
    if (latch == NULL || unscheduled == 0 || !oaf_mutex_lock(&latch->mutex))
    {
        return;
    }

    latch->failed = 1;
    if (unscheduled >= latch->remaining)
    {
        latch->remaining = 0;
    }
    else
    {
        latch->remaining -= unscheduled;
    }

    if (latch->remaining == 0)
    {
        oaf_cond_var_broadcast(&latch->completed);
    }

    oaf_mutex_unlock(&latch->mutex);
}

static int latch_await(OafParallelLatch* latch)
{
    int failed;

    if (latch == NULL || !oaf_mutex_lock(&latch->mutex))
    {
        return 0;
    }

    while (latch->remaining > 0)
    {
        if (!oaf_cond_var_wait(&latch->completed, &latch->mutex))
        {
            oaf_mutex_unlock(&latch->mutex);
            return 0;
        }
    }

    failed = latch->failed;
    oaf_mutex_unlock(&latch->mutex);
    return failed ? 0 : 1;
}

static void run_parallel_for_task(void* task_state)
{
    OafParallelForTask* task = (OafParallelForTask*)task_state;
    size_t index;
    int failed = 0;

    if (task == NULL || task->proc == NULL)
    {
        latch_mark_done(task != NULL ? task->latch : NULL, 1);
        return;
    }

    for (index = task->start; index < task->end; index++)
    {
        task->proc(index, task->state);
    }

    latch_mark_done(task->latch, failed);
}

static void run_parallel_map_task(void* task_state)
{
    OafParallelMapTask* task = (OafParallelMapTask*)task_state;
    size_t index;

    if (task == NULL || task->proc == NULL)
    {
        latch_mark_done(task != NULL ? task->latch : NULL, 1);
        return;
    }

    for (index = task->start; index < task->end; index++)
    {
        const void* input_element = task->input + (index * task->element_size);
        void* output_element = task->output + (index * task->element_size);
        task->proc(index, input_element, output_element, task->state);
    }

    latch_mark_done(task->latch, 0);
}

static void run_parallel_reduce_task_i64(void* task_state)
{
    OafParallelReduceTaskI64* task = (OafParallelReduceTaskI64*)task_state;
    size_t index;
    int64_t partial = 0;

    if (task == NULL || task->proc == NULL)
    {
        latch_mark_done(task != NULL ? task->latch : NULL, 1);
        return;
    }

    for (index = task->start; index < task->end; index++)
    {
        partial += task->proc(index, task->state);
    }

    task->partial = partial;
    latch_mark_done(task->latch, 0);
}

static int submit_tasks(
    OafThreadPool* pool,
    void* tasks,
    size_t task_count,
    size_t task_stride,
    OafThreadPoolTaskProc proc,
    OafParallelLatch* latch)
{
    size_t submitted = 0;

    while (submitted < task_count)
    {
        void* task = (unsigned char*)tasks + (submitted * task_stride);
        if (!oaf_thread_pool_submit(pool, proc, task))
        {
            break;
        }

        submitted++;
    }

    if (submitted < task_count)
    {
        latch_mark_unscheduled(latch, task_count - submitted);
    }

    return latch_await(latch);
}

int oaf_parallel_for(
    OafThreadPool* pool,
    size_t count,
    size_t chunk_size,
    OafParallelForProc proc,
    void* state)
{
    size_t actual_chunk_size;
    size_t task_count;
    OafParallelForTask* tasks;
    OafParallelLatch latch;
    size_t index;
    int ok;

    if (pool == NULL || proc == NULL)
    {
        return 0;
    }

    if (count == 0)
    {
        return 1;
    }

    actual_chunk_size = choose_chunk_size(pool, count, chunk_size);
    task_count = task_count_for_range(count, actual_chunk_size);
    if (task_count == 0)
    {
        return 1;
    }

    tasks = (OafParallelForTask*)malloc(sizeof(OafParallelForTask) * task_count);
    if (tasks == NULL || !latch_init(&latch, task_count))
    {
        free(tasks);
        return 0;
    }

    for (index = 0; index < task_count; index++)
    {
        size_t start = index * actual_chunk_size;
        size_t end = start + actual_chunk_size;
        if (end > count)
        {
            end = count;
        }

        tasks[index].start = start;
        tasks[index].end = end;
        tasks[index].proc = proc;
        tasks[index].state = state;
        tasks[index].latch = &latch;
    }

    ok = submit_tasks(pool, tasks, task_count, sizeof(OafParallelForTask), run_parallel_for_task, &latch);
    latch_destroy(&latch);
    free(tasks);
    return ok;
}

int oaf_parallel_map(
    OafThreadPool* pool,
    const void* input,
    void* output,
    size_t count,
    size_t element_size,
    size_t chunk_size,
    OafParallelMapProc proc,
    void* state)
{
    size_t actual_chunk_size;
    size_t task_count;
    OafParallelMapTask* tasks;
    OafParallelLatch latch;
    size_t index;
    int ok;

    if (pool == NULL || input == NULL || output == NULL || element_size == 0 || proc == NULL)
    {
        return 0;
    }

    if (count == 0)
    {
        return 1;
    }

    actual_chunk_size = choose_chunk_size(pool, count, chunk_size);
    task_count = task_count_for_range(count, actual_chunk_size);
    if (task_count == 0)
    {
        return 1;
    }

    tasks = (OafParallelMapTask*)malloc(sizeof(OafParallelMapTask) * task_count);
    if (tasks == NULL || !latch_init(&latch, task_count))
    {
        free(tasks);
        return 0;
    }

    for (index = 0; index < task_count; index++)
    {
        size_t start = index * actual_chunk_size;
        size_t end = start + actual_chunk_size;
        if (end > count)
        {
            end = count;
        }

        tasks[index].start = start;
        tasks[index].end = end;
        tasks[index].input = (const unsigned char*)input;
        tasks[index].output = (unsigned char*)output;
        tasks[index].element_size = element_size;
        tasks[index].proc = proc;
        tasks[index].state = state;
        tasks[index].latch = &latch;
    }

    ok = submit_tasks(pool, tasks, task_count, sizeof(OafParallelMapTask), run_parallel_map_task, &latch);
    latch_destroy(&latch);
    free(tasks);
    return ok;
}

int oaf_parallel_reduce_i64(
    OafThreadPool* pool,
    size_t count,
    size_t chunk_size,
    OafParallelReduceProcI64 proc,
    void* state,
    int64_t* out_result)
{
    size_t actual_chunk_size;
    size_t task_count;
    OafParallelReduceTaskI64* tasks;
    OafParallelLatch latch;
    size_t index;
    int ok;
    int64_t total = 0;

    if (pool == NULL || proc == NULL || out_result == NULL)
    {
        return 0;
    }

    if (count == 0)
    {
        *out_result = 0;
        return 1;
    }

    actual_chunk_size = choose_chunk_size(pool, count, chunk_size);
    task_count = task_count_for_range(count, actual_chunk_size);
    if (task_count == 0)
    {
        *out_result = 0;
        return 1;
    }

    tasks = (OafParallelReduceTaskI64*)malloc(sizeof(OafParallelReduceTaskI64) * task_count);
    if (tasks == NULL || !latch_init(&latch, task_count))
    {
        free(tasks);
        return 0;
    }

    for (index = 0; index < task_count; index++)
    {
        size_t start = index * actual_chunk_size;
        size_t end = start + actual_chunk_size;
        if (end > count)
        {
            end = count;
        }

        tasks[index].start = start;
        tasks[index].end = end;
        tasks[index].proc = proc;
        tasks[index].state = state;
        tasks[index].partial = 0;
        tasks[index].latch = &latch;
    }

    ok = submit_tasks(pool, tasks, task_count, sizeof(OafParallelReduceTaskI64), run_parallel_reduce_task_i64, &latch);
    if (ok)
    {
        for (index = 0; index < task_count; index++)
        {
            total += tasks[index].partial;
        }
        *out_result = total;
    }

    latch_destroy(&latch);
    free(tasks);
    return ok;
}
