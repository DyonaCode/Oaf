#include "oaf_stream.h"

void oaf_stream_init(
    OafStream* stream,
    void* state,
    OafStreamReadProc read,
    OafStreamWriteProc write,
    OafStreamSeekProc seek,
    OafStreamTellProc tell,
    OafStreamFlushProc flush,
    OafStreamCloseProc close)
{
    if (stream == NULL)
    {
        return;
    }

    stream->state = state;
    stream->read = read;
    stream->write = write;
    stream->seek = seek;
    stream->tell = tell;
    stream->flush = flush;
    stream->close = close;
}

size_t oaf_stream_read(OafStream* stream, void* buffer, size_t bytes)
{
    if (stream == NULL || stream->read == NULL)
    {
        return 0;
    }

    return stream->read(stream->state, buffer, bytes);
}

size_t oaf_stream_write(OafStream* stream, const void* buffer, size_t bytes)
{
    if (stream == NULL || stream->write == NULL)
    {
        return 0;
    }

    return stream->write(stream->state, buffer, bytes);
}

int oaf_stream_seek(OafStream* stream, long offset, int origin)
{
    if (stream == NULL || stream->seek == NULL)
    {
        return 0;
    }

    return stream->seek(stream->state, offset, origin);
}

long oaf_stream_tell(OafStream* stream)
{
    if (stream == NULL || stream->tell == NULL)
    {
        return -1;
    }

    return stream->tell(stream->state);
}

int oaf_stream_flush(OafStream* stream)
{
    if (stream == NULL || stream->flush == NULL)
    {
        return 0;
    }

    return stream->flush(stream->state);
}

int oaf_stream_close(OafStream* stream)
{
    if (stream == NULL || stream->close == NULL)
    {
        return 0;
    }

    return stream->close(stream->state);
}
