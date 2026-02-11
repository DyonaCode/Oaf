#include <stdlib.h>
#include "channel.h"

int oaf_channel_init(OafChannel* channel, size_t capacity)
{
    if (channel == NULL || capacity == 0)
    {
        return 0;
    }

    channel->buffer = (void**)malloc(sizeof(void*) * capacity);
    if (channel->buffer == NULL)
    {
        return 0;
    }

    channel->capacity = capacity;
    channel->count = 0;
    channel->send_index = 0;
    channel->recv_index = 0;
    channel->closed = 0;

    if (!oaf_mutex_init(&channel->mutex))
    {
        free(channel->buffer);
        channel->buffer = NULL;
        return 0;
    }

    if (!oaf_cond_var_init(&channel->not_empty))
    {
        oaf_mutex_destroy(&channel->mutex);
        free(channel->buffer);
        channel->buffer = NULL;
        return 0;
    }

    if (!oaf_cond_var_init(&channel->not_full))
    {
        oaf_cond_var_destroy(&channel->not_empty);
        oaf_mutex_destroy(&channel->mutex);
        free(channel->buffer);
        channel->buffer = NULL;
        return 0;
    }

    return 1;
}

void oaf_channel_destroy(OafChannel* channel)
{
    if (channel == NULL)
    {
        return;
    }

    if (channel->buffer != NULL)
    {
        free(channel->buffer);
        channel->buffer = NULL;
    }

    oaf_cond_var_destroy(&channel->not_full);
    oaf_cond_var_destroy(&channel->not_empty);
    oaf_mutex_destroy(&channel->mutex);
    channel->capacity = 0;
    channel->count = 0;
    channel->send_index = 0;
    channel->recv_index = 0;
    channel->closed = 1;
}

int oaf_channel_try_send(OafChannel* channel, void* value)
{
    int result = 0;

    if (channel == NULL || channel->buffer == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&channel->mutex))
    {
        return 0;
    }

    if (!channel->closed && channel->count < channel->capacity)
    {
        channel->buffer[channel->send_index] = value;
        channel->send_index = (channel->send_index + 1) % channel->capacity;
        channel->count++;
        result = 1;
        oaf_cond_var_signal(&channel->not_empty);
    }

    oaf_mutex_unlock(&channel->mutex);
    return result;
}

int oaf_channel_send(OafChannel* channel, void* value)
{
    if (channel == NULL || channel->buffer == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&channel->mutex))
    {
        return 0;
    }

    while (!channel->closed && channel->count == channel->capacity)
    {
        if (!oaf_cond_var_wait(&channel->not_full, &channel->mutex))
        {
            oaf_mutex_unlock(&channel->mutex);
            return 0;
        }
    }

    if (channel->closed)
    {
        oaf_mutex_unlock(&channel->mutex);
        return 0;
    }

    channel->buffer[channel->send_index] = value;
    channel->send_index = (channel->send_index + 1) % channel->capacity;
    channel->count++;
    oaf_cond_var_signal(&channel->not_empty);
    oaf_mutex_unlock(&channel->mutex);
    return 1;
}

int oaf_channel_try_recv(OafChannel* channel, void** out_value)
{
    int result = 0;

    if (channel == NULL || channel->buffer == NULL || out_value == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&channel->mutex))
    {
        return 0;
    }

    if (channel->count > 0)
    {
        *out_value = channel->buffer[channel->recv_index];
        channel->recv_index = (channel->recv_index + 1) % channel->capacity;
        channel->count--;
        result = 1;
        oaf_cond_var_signal(&channel->not_full);
    }

    oaf_mutex_unlock(&channel->mutex);
    return result;
}

int oaf_channel_recv(OafChannel* channel, void** out_value)
{
    if (channel == NULL || channel->buffer == NULL || out_value == NULL)
    {
        return 0;
    }

    if (!oaf_mutex_lock(&channel->mutex))
    {
        return 0;
    }

    while (channel->count == 0 && !channel->closed)
    {
        if (!oaf_cond_var_wait(&channel->not_empty, &channel->mutex))
        {
            oaf_mutex_unlock(&channel->mutex);
            return 0;
        }
    }

    if (channel->count == 0 && channel->closed)
    {
        oaf_mutex_unlock(&channel->mutex);
        return 0;
    }

    *out_value = channel->buffer[channel->recv_index];
    channel->recv_index = (channel->recv_index + 1) % channel->capacity;
    channel->count--;
    oaf_cond_var_signal(&channel->not_full);
    oaf_mutex_unlock(&channel->mutex);
    return 1;
}

void oaf_channel_close(OafChannel* channel)
{
    if (channel == NULL || channel->buffer == NULL)
    {
        return;
    }

    if (!oaf_mutex_lock(&channel->mutex))
    {
        return;
    }

    channel->closed = 1;
    oaf_cond_var_broadcast(&channel->not_empty);
    oaf_cond_var_broadcast(&channel->not_full);
    oaf_mutex_unlock(&channel->mutex);
}

size_t oaf_channel_count(const OafChannel* channel)
{
    if (channel == NULL)
    {
        return 0;
    }

    return channel->count;
}
