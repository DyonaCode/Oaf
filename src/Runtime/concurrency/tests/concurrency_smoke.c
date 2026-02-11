#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "scheduler.h"
#include "channel.h"
#include "atomic_ops.h"
#include "sync_primitives.h"

static OafAtomicI64 g_scheduler_counter;

static void* accumulate_task(void* args)
{
    int* value = (int*)args;
    oaf_atomic_i64_fetch_add(&g_scheduler_counter, *value);
    return NULL;
}

static int test_scheduler_and_work_stealing(void)
{
    OafThreadScheduler scheduler;
    OafLightweightThread* threads[6] = {0};
    int values[6] = {1, 2, 3, 4, 5, 6};
    size_t index;
    size_t guard = 64;
    const OafSchedulerStats* stats;

    if (!oaf_scheduler_init(&scheduler, 3))
    {
        return 0;
    }

    oaf_atomic_i64_init(&g_scheduler_counter, 0);

    for (index = 0; index < 6; index++)
    {
        threads[index] = oaf_scheduler_spawn(&scheduler, accumulate_task, &values[index]);
        if (threads[index] == NULL)
        {
            oaf_scheduler_shutdown(&scheduler);
            return 0;
        }
    }

    while (oaf_scheduler_pending_count(&scheduler) > 0 && guard > 0)
    {
        if (!oaf_scheduler_run_next(&scheduler, 0))
        {
            break;
        }
        guard--;
    }

    if (oaf_scheduler_pending_count(&scheduler) != 0)
    {
        oaf_scheduler_shutdown(&scheduler);
        return 0;
    }

    if (oaf_atomic_i64_load(&g_scheduler_counter) != 21)
    {
        oaf_scheduler_shutdown(&scheduler);
        return 0;
    }

    for (index = 0; index < 6; index++)
    {
        if (!oaf_lightweight_thread_is_done(threads[index]))
        {
            oaf_scheduler_shutdown(&scheduler);
            return 0;
        }
    }

    stats = oaf_scheduler_stats(&scheduler);
    if (stats == NULL || stats->stolen == 0)
    {
        oaf_scheduler_shutdown(&scheduler);
        return 0;
    }

    oaf_scheduler_shutdown(&scheduler);
    return 1;
}

static int test_channel_operations(void)
{
    OafChannel channel;
    int first = 10;
    int second = 20;
    int third = 30;
    void* received = NULL;

    if (!oaf_channel_init(&channel, 2))
    {
        return 0;
    }

    if (!oaf_channel_try_send(&channel, &first))
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    if (!oaf_channel_try_send(&channel, &second))
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    if (oaf_channel_try_send(&channel, &third))
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    if (!oaf_channel_recv(&channel, &received) || received != &first)
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    if (!oaf_channel_recv(&channel, &received) || received != &second)
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    oaf_channel_close(&channel);

    if (oaf_channel_recv(&channel, &received))
    {
        oaf_channel_destroy(&channel);
        return 0;
    }

    oaf_channel_destroy(&channel);
    return 1;
}

typedef struct WaitSignalState
{
    OafMutex mutex;
    OafCondVar cond_var;
    int ready;
    OafAtomicI64 wake_count;
} WaitSignalState;

static void* waiter_proc(void* args)
{
    WaitSignalState* state = (WaitSignalState*)args;

    oaf_mutex_lock(&state->mutex);
    while (!state->ready)
    {
        oaf_cond_var_wait(&state->cond_var, &state->mutex);
    }
    oaf_mutex_unlock(&state->mutex);
    oaf_atomic_i64_fetch_add(&state->wake_count, 1);
    return NULL;
}

static int test_sync_primitives(void)
{
    WaitSignalState state;
    pthread_t waiter;

    if (!oaf_mutex_init(&state.mutex) || !oaf_cond_var_init(&state.cond_var))
    {
        oaf_mutex_destroy(&state.mutex);
        oaf_cond_var_destroy(&state.cond_var);
        return 0;
    }

    state.ready = 0;
    oaf_atomic_i64_init(&state.wake_count, 0);

    if (pthread_create(&waiter, NULL, waiter_proc, &state) != 0)
    {
        oaf_cond_var_destroy(&state.cond_var);
        oaf_mutex_destroy(&state.mutex);
        return 0;
    }

    usleep(20000);
    oaf_mutex_lock(&state.mutex);
    state.ready = 1;
    oaf_cond_var_signal(&state.cond_var);
    oaf_mutex_unlock(&state.mutex);

    pthread_join(waiter, NULL);
    oaf_cond_var_destroy(&state.cond_var);
    oaf_mutex_destroy(&state.mutex);

    return oaf_atomic_i64_load(&state.wake_count) == 1;
}

static int test_atomic_operations(void)
{
    OafAtomicI64 value;
    int64_t expected;

    oaf_atomic_i64_init(&value, 10);
    if (oaf_atomic_i64_fetch_add(&value, 5) != 10)
    {
        return 0;
    }

    if (oaf_atomic_i64_fetch_sub(&value, 3) != 15)
    {
        return 0;
    }

    expected = 12;
    if (!oaf_atomic_i64_compare_exchange(&value, &expected, 20))
    {
        return 0;
    }

    expected = 12;
    if (oaf_atomic_i64_compare_exchange(&value, &expected, 30))
    {
        return 0;
    }

    return expected == 20 && oaf_atomic_i64_load(&value) == 20;
}

int main(void)
{
    int ok = 1;
    ok = ok && test_scheduler_and_work_stealing();
    ok = ok && test_channel_operations();
    ok = ok && test_sync_primitives();
    ok = ok && test_atomic_operations();

    if (!ok)
    {
        fprintf(stderr, "concurrency smoke tests failed\n");
        return 1;
    }

    printf("concurrency smoke tests passed\n");
    return 0;
}
