#ifndef OAFLANG_SCHEDULER_H
#define OAFLANG_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_SCHEDULER_MAX_WORKERS 8
#define OAF_SCHEDULER_QUEUE_CAPACITY 256
#define OAF_SCHEDULER_MAX_THREADS 512

typedef struct OafWorkStealingQueue
{
    OafLightweightThread* entries[OAF_SCHEDULER_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
} OafWorkStealingQueue;

typedef struct OafSchedulerStats
{
    size_t enqueued;
    size_t executed;
    size_t stolen;
    size_t failed_spawns;
} OafSchedulerStats;

typedef struct OafThreadScheduler
{
    OafWorkStealingQueue worker_queues[OAF_SCHEDULER_MAX_WORKERS];
    OafLightweightThread thread_pool[OAF_SCHEDULER_MAX_THREADS];
    size_t thread_count;
    size_t worker_count;
    size_t rr_worker;
    uint64_t next_thread_id;
    OafSchedulerStats stats;
} OafThreadScheduler;

int oaf_scheduler_init(OafThreadScheduler* scheduler, size_t worker_count);
void oaf_scheduler_shutdown(OafThreadScheduler* scheduler);
OafLightweightThread* oaf_scheduler_spawn(
    OafThreadScheduler* scheduler,
    OafLightweightThreadProc proc,
    void* proc_args);
int oaf_scheduler_run_next(OafThreadScheduler* scheduler, size_t worker_index);
size_t oaf_scheduler_run_all(OafThreadScheduler* scheduler);
int oaf_scheduler_steal(
    OafThreadScheduler* scheduler,
    size_t thief_worker_index,
    OafLightweightThread** thread_out);
size_t oaf_scheduler_pending_count(const OafThreadScheduler* scheduler);
const OafSchedulerStats* oaf_scheduler_stats(const OafThreadScheduler* scheduler);

#ifdef __cplusplus
}
#endif

#endif
