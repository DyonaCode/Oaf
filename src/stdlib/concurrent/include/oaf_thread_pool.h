#ifndef OAF_STDLIB_THREAD_POOL_H
#define OAF_STDLIB_THREAD_POOL_H

#include <stddef.h>
#include <pthread.h>
#include "sync_primitives.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OafThreadPoolTaskProc)(void* state);

typedef struct OafThreadPoolTask
{
    OafThreadPoolTaskProc proc;
    void* state;
} OafThreadPoolTask;

typedef struct OafThreadPoolStats
{
    size_t submitted;
    size_t completed;
    size_t rejected;
} OafThreadPoolStats;

typedef struct OafThreadPool
{
    pthread_t* workers;
    size_t worker_count;

    OafThreadPoolTask* queue;
    size_t queue_capacity;
    size_t queue_head;
    size_t queue_tail;
    size_t queue_count;
    size_t active_workers;

    int shutting_down;

    OafMutex mutex;
    OafCondVar has_work;
    OafCondVar has_space;
    OafCondVar idle;

    OafThreadPoolStats stats;
} OafThreadPool;

int oaf_thread_pool_init(OafThreadPool* pool, size_t worker_count, size_t queue_capacity);
void oaf_thread_pool_shutdown(OafThreadPool* pool);

int oaf_thread_pool_submit(OafThreadPool* pool, OafThreadPoolTaskProc proc, void* state);
int oaf_thread_pool_try_submit(OafThreadPool* pool, OafThreadPoolTaskProc proc, void* state);
int oaf_thread_pool_wait_idle(OafThreadPool* pool);

size_t oaf_thread_pool_worker_count(const OafThreadPool* pool);
const OafThreadPoolStats* oaf_thread_pool_stats(const OafThreadPool* pool);

#ifdef __cplusplus
}
#endif

#endif
