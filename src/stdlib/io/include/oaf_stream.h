#ifndef OAFLANG_STDLIB_STREAM_H
#define OAFLANG_STDLIB_STREAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*OafStreamReadProc)(void* state, void* buffer, size_t bytes);
typedef size_t (*OafStreamWriteProc)(void* state, const void* buffer, size_t bytes);
typedef int (*OafStreamSeekProc)(void* state, long offset, int origin);
typedef long (*OafStreamTellProc)(void* state);
typedef int (*OafStreamFlushProc)(void* state);
typedef int (*OafStreamCloseProc)(void* state);

typedef struct OafStream
{
    void* state;
    OafStreamReadProc read;
    OafStreamWriteProc write;
    OafStreamSeekProc seek;
    OafStreamTellProc tell;
    OafStreamFlushProc flush;
    OafStreamCloseProc close;
} OafStream;

void oaf_stream_init(
    OafStream* stream,
    void* state,
    OafStreamReadProc read,
    OafStreamWriteProc write,
    OafStreamSeekProc seek,
    OafStreamTellProc tell,
    OafStreamFlushProc flush,
    OafStreamCloseProc close);
size_t oaf_stream_read(OafStream* stream, void* buffer, size_t bytes);
size_t oaf_stream_write(OafStream* stream, const void* buffer, size_t bytes);
int oaf_stream_seek(OafStream* stream, long offset, int origin);
long oaf_stream_tell(OafStream* stream);
int oaf_stream_flush(OafStream* stream);
int oaf_stream_close(OafStream* stream);

#ifdef __cplusplus
}
#endif

#endif
