#ifndef OAFLANG_CHANNEL_H
#define OAFLANG_CHANNEL_H

#include <stddef.h>
#include "sync_primitives.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafChannel
{
    void** buffer;
    size_t capacity;
    size_t count;
    size_t send_index;
    size_t recv_index;
    int closed;
    OafMutex mutex;
    OafCondVar not_empty;
    OafCondVar not_full;
} OafChannel;

int oaf_channel_init(OafChannel* channel, size_t capacity);
void oaf_channel_destroy(OafChannel* channel);
int oaf_channel_send(OafChannel* channel, void* value);
int oaf_channel_try_send(OafChannel* channel, void* value);
int oaf_channel_recv(OafChannel* channel, void** out_value);
int oaf_channel_try_recv(OafChannel* channel, void** out_value);
void oaf_channel_close(OafChannel* channel);
size_t oaf_channel_count(const OafChannel* channel);

#ifdef __cplusplus
}
#endif

#endif
