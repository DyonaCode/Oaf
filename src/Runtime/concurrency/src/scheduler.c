#include <stddef.h>
#include "scheduler.h"

static void queue_init(OafWorkStealingQueue* queue)
{
    if (queue == NULL)
    {
        return;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

static int queue_push_back(OafWorkStealingQueue* queue, OafLightweightThread* thread)
{
    if (queue == NULL || thread == NULL || queue->count >= OAF_SCHEDULER_QUEUE_CAPACITY)
    {
        return 0;
    }

    queue->entries[queue->tail] = thread;
    queue->tail = (queue->tail + 1) % OAF_SCHEDULER_QUEUE_CAPACITY;
    queue->count++;
    return 1;
}

static OafLightweightThread* queue_pop_front(OafWorkStealingQueue* queue)
{
    OafLightweightThread* thread;

    if (queue == NULL || queue->count == 0)
    {
        return NULL;
    }

    thread = queue->entries[queue->head];
    queue->entries[queue->head] = NULL;
    queue->head = (queue->head + 1) % OAF_SCHEDULER_QUEUE_CAPACITY;
    queue->count--;
    return thread;
}

static OafLightweightThread* queue_pop_back(OafWorkStealingQueue* queue)
{
    OafLightweightThread* thread;

    if (queue == NULL || queue->count == 0)
    {
        return NULL;
    }

    queue->tail = (queue->tail + OAF_SCHEDULER_QUEUE_CAPACITY - 1) % OAF_SCHEDULER_QUEUE_CAPACITY;
    thread = queue->entries[queue->tail];
    queue->entries[queue->tail] = NULL;
    queue->count--;
    return thread;
}

int oaf_scheduler_init(OafThreadScheduler* scheduler, size_t worker_count)
{
    size_t worker_index;

    if (scheduler == NULL)
    {
        return 0;
    }

    if (worker_count == 0)
    {
        worker_count = 1;
    }

    if (worker_count > OAF_SCHEDULER_MAX_WORKERS)
    {
        worker_count = OAF_SCHEDULER_MAX_WORKERS;
    }

    scheduler->thread_count = 0;
    scheduler->worker_count = worker_count;
    scheduler->rr_worker = 0;
    scheduler->next_thread_id = 1;
    scheduler->stats.enqueued = 0;
    scheduler->stats.executed = 0;
    scheduler->stats.stolen = 0;
    scheduler->stats.failed_spawns = 0;

    for (worker_index = 0; worker_index < OAF_SCHEDULER_MAX_WORKERS; worker_index++)
    {
        queue_init(&scheduler->worker_queues[worker_index]);
    }

    return 1;
}

void oaf_scheduler_shutdown(OafThreadScheduler* scheduler)
{
    size_t worker_index;

    if (scheduler == NULL)
    {
        return;
    }

    scheduler->thread_count = 0;
    scheduler->rr_worker = 0;

    for (worker_index = 0; worker_index < OAF_SCHEDULER_MAX_WORKERS; worker_index++)
    {
        queue_init(&scheduler->worker_queues[worker_index]);
    }
}

OafLightweightThread* oaf_scheduler_spawn(
    OafThreadScheduler* scheduler,
    OafLightweightThreadProc proc,
    void* proc_args)
{
    OafLightweightThread* thread;
    size_t target_worker;

    if (scheduler == NULL || proc == NULL)
    {
        return NULL;
    }

    if (scheduler->thread_count >= OAF_SCHEDULER_MAX_THREADS)
    {
        scheduler->stats.failed_spawns++;
        return NULL;
    }

    thread = &scheduler->thread_pool[scheduler->thread_count];
    scheduler->thread_count++;
    oaf_lightweight_thread_init(thread, scheduler->next_thread_id, proc, proc_args);
    scheduler->next_thread_id++;

    target_worker = scheduler->rr_worker % scheduler->worker_count;
    scheduler->rr_worker++;

    if (!queue_push_back(&scheduler->worker_queues[target_worker], thread))
    {
        scheduler->stats.failed_spawns++;
        thread->state = OAF_THREAD_STATE_FAILED;
        return NULL;
    }

    scheduler->stats.enqueued++;
    return thread;
}

int oaf_scheduler_steal(
    OafThreadScheduler* scheduler,
    size_t thief_worker_index,
    OafLightweightThread** thread_out)
{
    size_t victim_offset;

    if (scheduler == NULL || thread_out == NULL || thief_worker_index >= scheduler->worker_count)
    {
        return 0;
    }

    for (victim_offset = 1; victim_offset < scheduler->worker_count; victim_offset++)
    {
        size_t victim_index = (thief_worker_index + victim_offset) % scheduler->worker_count;
        OafWorkStealingQueue* victim_queue = &scheduler->worker_queues[victim_index];
        OafLightweightThread* stolen;

        if (victim_queue->count == 0)
        {
            continue;
        }

        stolen = queue_pop_back(victim_queue);
        if (stolen != NULL)
        {
            scheduler->stats.stolen++;
            *thread_out = stolen;
            return 1;
        }
    }

    return 0;
}

int oaf_scheduler_run_next(OafThreadScheduler* scheduler, size_t worker_index)
{
    OafLightweightThread* thread;

    if (scheduler == NULL || worker_index >= scheduler->worker_count)
    {
        return 0;
    }

    thread = queue_pop_front(&scheduler->worker_queues[worker_index]);
    if (thread == NULL)
    {
        OafLightweightThread* stolen = NULL;
        if (!oaf_scheduler_steal(scheduler, worker_index, &stolen))
        {
            return 0;
        }

        thread = stolen;
    }

    if (!oaf_lightweight_thread_run(thread))
    {
        thread->state = OAF_THREAD_STATE_FAILED;
        return 0;
    }

    scheduler->stats.executed++;
    return 1;
}

size_t oaf_scheduler_run_all(OafThreadScheduler* scheduler)
{
    size_t total_executed = 0;
    size_t guard = OAF_SCHEDULER_MAX_THREADS * 2;

    if (scheduler == NULL)
    {
        return 0;
    }

    while (oaf_scheduler_pending_count(scheduler) > 0 && guard > 0)
    {
        size_t worker_index;
        size_t executed_this_round = 0;

        for (worker_index = 0; worker_index < scheduler->worker_count; worker_index++)
        {
            if (oaf_scheduler_run_next(scheduler, worker_index))
            {
                executed_this_round++;
                total_executed++;
            }
        }

        if (executed_this_round == 0)
        {
            break;
        }

        guard--;
    }

    return total_executed;
}

size_t oaf_scheduler_pending_count(const OafThreadScheduler* scheduler)
{
    size_t worker_index;
    size_t pending = 0;

    if (scheduler == NULL)
    {
        return 0;
    }

    for (worker_index = 0; worker_index < scheduler->worker_count; worker_index++)
    {
        pending += scheduler->worker_queues[worker_index].count;
    }

    return pending;
}

const OafSchedulerStats* oaf_scheduler_stats(const OafThreadScheduler* scheduler)
{
    if (scheduler == NULL)
    {
        return NULL;
    }

    return &scheduler->stats;
}
