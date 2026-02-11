#include <stddef.h>
#include "thread.h"

void oaf_lightweight_thread_init(
    OafLightweightThread* thread,
    uint64_t id,
    OafLightweightThreadProc proc,
    void* proc_args)
{
    if (thread == NULL)
    {
        return;
    }

    thread->id = id;
    thread->state = OAF_THREAD_STATE_READY;
    thread->proc = proc;
    thread->proc_args = proc_args;
    thread->result = NULL;
    thread->next = NULL;
}

int oaf_lightweight_thread_run(OafLightweightThread* thread)
{
    if (thread == NULL || thread->proc == NULL)
    {
        return 0;
    }

    if (thread->state != OAF_THREAD_STATE_READY)
    {
        return 0;
    }

    thread->state = OAF_THREAD_STATE_RUNNING;
    thread->result = thread->proc(thread->proc_args);
    thread->state = OAF_THREAD_STATE_COMPLETED;
    return 1;
}

int oaf_lightweight_thread_is_done(const OafLightweightThread* thread)
{
    if (thread == NULL)
    {
        return 0;
    }

    return thread->state == OAF_THREAD_STATE_COMPLETED
        || thread->state == OAF_THREAD_STATE_FAILED
        || thread->state == OAF_THREAD_STATE_CANCELLED;
}
