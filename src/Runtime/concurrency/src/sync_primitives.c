#include "sync_primitives.h"

int oaf_mutex_init(OafMutex* mutex)
{
    if (mutex == NULL)
    {
        return 0;
    }

    if (pthread_mutex_init(&mutex->handle, NULL) != 0)
    {
        mutex->initialized = 0;
        return 0;
    }

    mutex->initialized = 1;
    return 1;
}

void oaf_mutex_destroy(OafMutex* mutex)
{
    if (mutex == NULL || !mutex->initialized)
    {
        return;
    }

    pthread_mutex_destroy(&mutex->handle);
    mutex->initialized = 0;
}

int oaf_mutex_lock(OafMutex* mutex)
{
    if (mutex == NULL || !mutex->initialized)
    {
        return 0;
    }

    return pthread_mutex_lock(&mutex->handle) == 0;
}

int oaf_mutex_unlock(OafMutex* mutex)
{
    if (mutex == NULL || !mutex->initialized)
    {
        return 0;
    }

    return pthread_mutex_unlock(&mutex->handle) == 0;
}

int oaf_cond_var_init(OafCondVar* cond_var)
{
    if (cond_var == NULL)
    {
        return 0;
    }

    if (pthread_cond_init(&cond_var->handle, NULL) != 0)
    {
        cond_var->initialized = 0;
        return 0;
    }

    cond_var->initialized = 1;
    return 1;
}

void oaf_cond_var_destroy(OafCondVar* cond_var)
{
    if (cond_var == NULL || !cond_var->initialized)
    {
        return;
    }

    pthread_cond_destroy(&cond_var->handle);
    cond_var->initialized = 0;
}

int oaf_cond_var_wait(OafCondVar* cond_var, OafMutex* mutex)
{
    if (cond_var == NULL || mutex == NULL || !cond_var->initialized || !mutex->initialized)
    {
        return 0;
    }

    return pthread_cond_wait(&cond_var->handle, &mutex->handle) == 0;
}

int oaf_cond_var_signal(OafCondVar* cond_var)
{
    if (cond_var == NULL || !cond_var->initialized)
    {
        return 0;
    }

    return pthread_cond_signal(&cond_var->handle) == 0;
}

int oaf_cond_var_broadcast(OafCondVar* cond_var)
{
    if (cond_var == NULL || !cond_var->initialized)
    {
        return 0;
    }

    return pthread_cond_broadcast(&cond_var->handle) == 0;
}
