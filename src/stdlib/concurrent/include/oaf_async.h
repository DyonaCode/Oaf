#ifndef OAFLANG_STDLIB_ASYNC_H
#define OAFLANG_STDLIB_ASYNC_H

#include "sync_primitives.h"
#include "oaf_thread_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*OafAsyncProc)(void* state);

typedef struct OafFuture
{
    OafMutex mutex;
    OafCondVar completed;
    int initialized;
    int is_ready;
    int is_failed;
    void* result;
} OafFuture;

int oaf_future_init(OafFuture* future);
void oaf_future_destroy(OafFuture* future);

int oaf_future_is_ready(OafFuture* future);
int oaf_future_try_get(OafFuture* future, void** out_result);
int oaf_future_await(OafFuture* future, void** out_result);

int oaf_async_submit(OafThreadPool* pool, OafAsyncProc proc, void* state, OafFuture* out_future);

#ifdef __cplusplus
}
#endif

#endif
