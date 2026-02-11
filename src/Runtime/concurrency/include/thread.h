#ifndef OAFLANG_THREAD_H
#define OAFLANG_THREAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OafThreadState
{
    OAF_THREAD_STATE_NEW = 0,
    OAF_THREAD_STATE_READY = 1,
    OAF_THREAD_STATE_RUNNING = 2,
    OAF_THREAD_STATE_COMPLETED = 3,
    OAF_THREAD_STATE_FAILED = 4,
    OAF_THREAD_STATE_CANCELLED = 5
} OafThreadState;

typedef void* (*OafLightweightThreadProc)(void* args);

typedef struct OafLightweightThread
{
    uint64_t id;
    OafThreadState state;
    OafLightweightThreadProc proc;
    void* proc_args;
    void* result;
    struct OafLightweightThread* next;
} OafLightweightThread;

void oaf_lightweight_thread_init(
    OafLightweightThread* thread,
    uint64_t id,
    OafLightweightThreadProc proc,
    void* proc_args);
int oaf_lightweight_thread_run(OafLightweightThread* thread);
int oaf_lightweight_thread_is_done(const OafLightweightThread* thread);

#ifdef __cplusplus
}
#endif

#endif
