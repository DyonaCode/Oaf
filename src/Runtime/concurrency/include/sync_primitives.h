#ifndef OAF_SYNC_PRIMITIVES_H
#define OAF_SYNC_PRIMITIVES_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafMutex
{
    pthread_mutex_t handle;
    int initialized;
} OafMutex;

typedef struct OafCondVar
{
    pthread_cond_t handle;
    int initialized;
} OafCondVar;

int oaf_mutex_init(OafMutex* mutex);
void oaf_mutex_destroy(OafMutex* mutex);
int oaf_mutex_lock(OafMutex* mutex);
int oaf_mutex_unlock(OafMutex* mutex);

int oaf_cond_var_init(OafCondVar* cond_var);
void oaf_cond_var_destroy(OafCondVar* cond_var);
int oaf_cond_var_wait(OafCondVar* cond_var, OafMutex* mutex);
int oaf_cond_var_signal(OafCondVar* cond_var);
int oaf_cond_var_broadcast(OafCondVar* cond_var);

#ifdef __cplusplus
}
#endif

#endif
