#include <stdlib.h>
#include "oaf_thread_pool.h"

static int queue_push(OafThreadPool* pool, OafThreadPoolTaskProc proc, void* state)
{
    if (pool->queue_count >= pool->queue_capacity)
    {
        return 0;
    }

    pool->queue[pool->queue_tail].proc = proc;
    pool->queue[pool->queue_tail].state = state;
    pool->queue_tail = (pool->queue_tail + 1u) % pool->queue_capacity;
    pool->queue_count++;
    return 1;
}

static int queue_pop(OafThreadPool* pool, OafThreadPoolTask* out_task)
{
    if (pool->queue_count == 0 || out_task == NULL)
    {
        return 0;
    }

    *out_task = pool->queue[pool->queue_head];
    pool->queue[pool->queue_head].proc = NULL;
    pool->queue[pool->queue_head].state = NULL;
    pool->queue_head = (pool->queue_head + 1u) % pool->queue_capacity;
    pool->queue_count--;
    return 1;
}

static void* worker_main(void* state)
{
    OafThreadPool* pool = (OafThreadPool*)state;

    while (1)
    {
        OafThreadPoolTask task;

        if (!oaf_mutex_lock(&pool->mutex))
        {
            break;
        }

        while (pool->queue_count == 0 && !pool->shutting_down)
        {
            if (!oaf_cond_var_wait(&pool->has_work, &pool->mutex))
            {
                oaf_mutex_unlock(&pool->mutex);
                return NULL;
            }
        }

        if (pool->queue_count == 0 && pool->shutting_down)
        {
            oaf_mutex_unlock(&pool->mutex);
            break;
        }

        if (!queue_pop(pool, &task))
        {
            oaf_mutex_unlock(&pool->mutex);
            continue;
        }

        pool->active_workers++;
        oaf_cond_var_signal(&pool->has_space);
        oaf_mutex_unlock(&pool->mutex);

        if (task.proc != NULL)
        {
            task.proc(task.state);
        }

        if (!oaf_mutex_lock(&pool->mutex))
        {
            break;
        }

        if (pool->active_workers > 0)
        {
            pool->active_workers--;
        }
        pool->stats.completed++;

        if (pool->queue_count == 0 && pool->active_workers == 0)
        {
            oaf_cond_var_broadcast(&pool->idle);
        }

        oaf_mutex_unlock(&pool->mutex);
    }

    return NULL;
}

static void reset_pool(OafThreadPool* pool)
{
    pool->workers = NULL;
    pool->worker_count = 0;
    pool->queue = NULL;
    pool->queue_capacity = 0;
    pool->queue_head = 0;
    pool->queue_tail = 0;
    pool->queue_count = 0;
    pool->active_workers = 0;
    pool->shutting_down = 0;
    pool->stats.submitted = 0;
    pool->stats.completed = 0;
    pool->stats.rejected = 0;
}

int oaf_thread_pool_init(OafThreadPool* pool, size_t worker_count, size_t queue_capacity)
{
    size_t index;

    if (pool == NULL || worker_count == 0 || queue_capacity == 0)
    {
        return 0;
    }

    reset_pool(pool);

    pool->workers = (pthread_t*)malloc(sizeof(pthread_t) * worker_count);
    if (pool->workers == NULL)
    {
        reset_pool(pool);
        return 0;
    }

    pool->queue = (OafThreadPoolTask*)malloc(sizeof(OafThreadPoolTask) * queue_capacity);
    if (pool->queue == NULL)
    {
        free(pool->workers);
        reset_pool(pool);
        return 0;
    }

    pool->worker_count = worker_count;
    pool->queue_capacity = queue_capacity;

    if (!oaf_mutex_init(&pool->mutex)
        || !oaf_cond_var_init(&pool->has_work)
        || !oaf_cond_var_init(&pool->has_space)
        || !oaf_cond_var_init(&pool->idle))
    {
        oaf_cond_var_destroy(&pool->idle);
        oaf_cond_var_destroy(&pool->has_space);
        oaf_cond_var_destroy(&pool->has_work);
        oaf_mutex_destroy(&pool->mutex);
        free(pool->queue);
        free(pool->workers);
        reset_pool(pool);
        return 0;
    }

    for (index = 0; index < pool->worker_count; index++)
    {
        if (pthread_create(&pool->workers[index], NULL, worker_main, pool) != 0)
        {
            size_t join_index;

            oaf_mutex_lock(&pool->mutex);
            pool->shutting_down = 1;
            oaf_cond_var_broadcast(&pool->has_work);
            oaf_mutex_unlock(&pool->mutex);

            for (join_index = 0; join_index < index; join_index++)
            {
                pthread_join(pool->workers[join_index], NULL);
            }

            oaf_cond_var_destroy(&pool->idle);
            oaf_cond_var_destroy(&pool->has_space);
            oaf_cond_var_destroy(&pool->has_work);
            oaf_mutex_destroy(&pool->mutex);
            free(pool->queue);
            free(pool->workers);
            reset_pool(pool);
            return 0;
        }
    }

    return 1;
}

void oaf_thread_pool_shutdown(OafThreadPool* pool)
{
    size_t index;

    if (pool == NULL)
    {
        return;
    }

    if (pool->workers == NULL)
    {
        reset_pool(pool);
        return;
    }

    if (oaf_mutex_lock(&pool->mutex))
    {
        pool->shutting_down = 1;
        oaf_cond_var_broadcast(&pool->has_work);
        oaf_cond_var_broadcast(&pool->has_space);
        oaf_cond_var_broadcast(&pool->idle);
        oaf_mutex_unlock(&pool->mutex);
    }

    for (index = 0; index < pool->worker_count; index++)
    {
        pthread_join(pool->workers[index], NULL);
    }

    oaf_cond_var_destroy(&pool->idle);
    oaf_cond_var_destroy(&pool->has_space);
    oaf_cond_var_destroy(&pool->has_work);
    oaf_mutex_destroy(&pool->mutex);

    free(pool->queue);
    free(pool->workers);
    reset_pool(pool);
}

int oaf_thread_pool_submit(OafThreadPool* pool, OafThreadPoolTaskProc proc, void* state)
{
    int success = 0;

    if (pool == NULL || proc == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&pool->mutex))
    {
        return 0;
    }

    while (!pool->shutting_down && pool->queue_count >= pool->queue_capacity)
    {
        if (!oaf_cond_var_wait(&pool->has_space, &pool->mutex))
        {
            oaf_mutex_unlock(&pool->mutex);
            return 0;
        }
    }

    if (!pool->shutting_down)
    {
        success = queue_push(pool, proc, state);
        if (success)
        {
            pool->stats.submitted++;
            oaf_cond_var_signal(&pool->has_work);
        }
    }

    if (!success)
    {
        pool->stats.rejected++;
    }

    oaf_mutex_unlock(&pool->mutex);
    return success;
}

int oaf_thread_pool_try_submit(OafThreadPool* pool, OafThreadPoolTaskProc proc, void* state)
{
    int success = 0;

    if (pool == NULL || proc == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&pool->mutex))
    {
        return 0;
    }

    if (!pool->shutting_down)
    {
        success = queue_push(pool, proc, state);
        if (success)
        {
            pool->stats.submitted++;
            oaf_cond_var_signal(&pool->has_work);
        }
    }

    if (!success)
    {
        pool->stats.rejected++;
    }

    oaf_mutex_unlock(&pool->mutex);
    return success;
}

int oaf_thread_pool_wait_idle(OafThreadPool* pool)
{
    if (pool == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&pool->mutex))
    {
        return 0;
    }

    while (pool->queue_count > 0 || pool->active_workers > 0)
    {
        if (!oaf_cond_var_wait(&pool->idle, &pool->mutex))
        {
            oaf_mutex_unlock(&pool->mutex);
            return 0;
        }
    }

    oaf_mutex_unlock(&pool->mutex);
    return 1;
}

size_t oaf_thread_pool_worker_count(const OafThreadPool* pool)
{
    if (pool == NULL)
    {
        return 0;
    }

    return pool->worker_count;
}

const OafThreadPoolStats* oaf_thread_pool_stats(const OafThreadPool* pool)
{
    if (pool == NULL)
    {
        return NULL;
    }

    return &pool->stats;
}
