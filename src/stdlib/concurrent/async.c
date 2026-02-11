#include <stdlib.h>
#include "oaf_async.h"

typedef struct OafAsyncTask
{
    OafAsyncProc proc;
    void* state;
    OafFuture* future;
} OafAsyncTask;

static void future_finish(OafFuture* future, void* result, int failed)
{
    if (future == NULL)
    {
        return;
    }

    if (!oaf_mutex_lock(&future->mutex))
    {
        return;
    }

    future->result = result;
    future->is_failed = failed;
    future->is_ready = 1;
    oaf_cond_var_broadcast(&future->completed);
    oaf_mutex_unlock(&future->mutex);
}

static void run_async_task(void* task_state)
{
    OafAsyncTask* task = (OafAsyncTask*)task_state;
    void* result = NULL;
    int failed = 0;

    if (task == NULL || task->future == NULL)
    {
        free(task);
        return;
    }

    if (task->proc == NULL)
    {
        failed = 1;
    }
    else
    {
        result = task->proc(task->state);
    }

    future_finish(task->future, result, failed);
    free(task);
}

int oaf_future_init(OafFuture* future)
{
    if (future == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_init(&future->mutex))
    {
        return 0;
    }

    if (!oaf_cond_var_init(&future->completed))
    {
        oaf_mutex_destroy(&future->mutex);
        return 0;
    }

    future->initialized = 1;
    future->is_ready = 0;
    future->is_failed = 0;
    future->result = NULL;
    return 1;
}

void oaf_future_destroy(OafFuture* future)
{
    if (future == NULL || !future->initialized)
    {
        return;
    }

    oaf_cond_var_destroy(&future->completed);
    oaf_mutex_destroy(&future->mutex);
    future->initialized = 0;
    future->is_ready = 0;
    future->is_failed = 0;
    future->result = NULL;
}

int oaf_future_is_ready(OafFuture* future)
{
    int is_ready;

    if (future == NULL || !future->initialized)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&future->mutex))
    {
        return 0;
    }

    is_ready = future->is_ready;
    oaf_mutex_unlock(&future->mutex);
    return is_ready;
}

int oaf_future_try_get(OafFuture* future, void** out_result)
{
    int is_ready;

    if (future == NULL || !future->initialized || out_result == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&future->mutex))
    {
        return 0;
    }

    is_ready = future->is_ready && !future->is_failed;
    if (is_ready)
    {
        *out_result = future->result;
    }

    oaf_mutex_unlock(&future->mutex);
    return is_ready;
}

int oaf_future_await(OafFuture* future, void** out_result)
{
    if (future == NULL || !future->initialized || out_result == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&future->mutex))
    {
        return 0;
    }

    while (!future->is_ready)
    {
        if (!oaf_cond_var_wait(&future->completed, &future->mutex))
        {
            oaf_mutex_unlock(&future->mutex);
            return 0;
        }
    }

    *out_result = future->result;
    oaf_mutex_unlock(&future->mutex);
    return future->is_failed ? 0 : 1;
}

int oaf_async_submit(OafThreadPool* pool, OafAsyncProc proc, void* state, OafFuture* out_future)
{
    OafAsyncTask* task;

    if (pool == NULL || proc == NULL || out_future == NULL)
    {
        return 0;
    }

    if (!oaf_future_init(out_future))
    {
        return 0;
    }

    task = (OafAsyncTask*)malloc(sizeof(OafAsyncTask));
    if (task == NULL)
    {
        oaf_future_destroy(out_future);
        return 0;
    }

    task->proc = proc;
    task->state = state;
    task->future = out_future;

    if (!oaf_thread_pool_submit(pool, run_async_task, task))
    {
        free(task);
        oaf_future_destroy(out_future);
        return 0;
    }

    return 1;
}
