#include <stdio.h>
#include <string.h>
#include "oaf_file.h"

static FILE* as_handle(OafFile* file)
{
    if (file == NULL)
    {
        return NULL;
    }

    return (FILE*)file->handle;
}

static size_t file_read_proc(void* state, void* buffer, size_t bytes)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_read(file, buffer, bytes);
}

static size_t file_write_proc(void* state, const void* buffer, size_t bytes)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_write(file, buffer, bytes);
}

static int file_seek_proc(void* state, long offset, int origin)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_seek(file, offset, origin);
}

static long file_tell_proc(void* state)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_tell(file);
}

static int file_flush_proc(void* state)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_flush(file);
}

static int file_close_proc(void* state)
{
    OafFile* file = (OafFile*)state;
    return oaf_file_close(file);
}

int oaf_file_open(OafFile* file, const char* path, const char* mode)
{
    if (file == NULL || path == NULL || mode == NULL)
    {
        return 0;
    }

    file->handle = (void*)fopen(path, mode);
    return file->handle != NULL;
}

int oaf_file_is_open(const OafFile* file)
{
    return file != NULL && file->handle != NULL;
}

size_t oaf_file_read(OafFile* file, void* buffer, size_t bytes)
{
    FILE* handle = as_handle(file);
    if (handle == NULL || buffer == NULL)
    {
        return 0;
    }

    return fread(buffer, 1u, bytes, handle);
}

size_t oaf_file_write(OafFile* file, const void* buffer, size_t bytes)
{
    FILE* handle = as_handle(file);
    if (handle == NULL || buffer == NULL)
    {
        return 0;
    }

    return fwrite(buffer, 1u, bytes, handle);
}

int oaf_file_seek(OafFile* file, long offset, int origin)
{
    FILE* handle = as_handle(file);
    if (handle == NULL)
    {
        return 0;
    }

    return fseek(handle, offset, origin) == 0;
}

long oaf_file_tell(OafFile* file)
{
    FILE* handle = as_handle(file);
    if (handle == NULL)
    {
        return -1;
    }

    return ftell(handle);
}

int oaf_file_flush(OafFile* file)
{
    FILE* handle = as_handle(file);
    if (handle == NULL)
    {
        return 0;
    }

    return fflush(handle) == 0;
}

int oaf_file_close(OafFile* file)
{
    FILE* handle = as_handle(file);
    if (handle == NULL)
    {
        return 0;
    }

    file->handle = NULL;
    return fclose(handle) == 0;
}

int oaf_file_read_all_text(
    const char* path,
    OafAllocator* allocator,
    char** out_text,
    size_t* out_length)
{
    OafFile file;
    long end_offset;
    size_t length;
    char* buffer;

    if (path == NULL || allocator == NULL || out_text == NULL)
    {
        return 0;
    }

    *out_text = NULL;
    if (out_length != NULL)
    {
        *out_length = 0;
    }

    if (!oaf_file_open(&file, path, "rb"))
    {
        return 0;
    }

    if (!oaf_file_seek(&file, 0, SEEK_END))
    {
        oaf_file_close(&file);
        return 0;
    }

    end_offset = oaf_file_tell(&file);
    if (end_offset < 0 || !oaf_file_seek(&file, 0, SEEK_SET))
    {
        oaf_file_close(&file);
        return 0;
    }

    length = (size_t)end_offset;
    buffer = (char*)oaf_allocator_alloc(allocator, length + 1u, _Alignof(char));
    if (buffer == NULL)
    {
        oaf_file_close(&file);
        return 0;
    }

    if (length > 0)
    {
        size_t read = oaf_file_read(&file, buffer, length);
        if (read != length)
        {
            oaf_allocator_free(allocator, buffer);
            oaf_file_close(&file);
            return 0;
        }
    }

    buffer[length] = '\0';
    oaf_file_close(&file);
    *out_text = buffer;
    if (out_length != NULL)
    {
        *out_length = length;
    }

    return 1;
}

int oaf_file_write_all_text(const char* path, const char* text, size_t length)
{
    OafFile file;
    size_t written;

    if (path == NULL || text == NULL)
    {
        return 0;
    }

    if (!oaf_file_open(&file, path, "wb"))
    {
        return 0;
    }

    written = oaf_file_write(&file, text, length);
    if (written != length)
    {
        oaf_file_close(&file);
        return 0;
    }

    return oaf_file_close(&file);
}

void oaf_stream_from_file(OafFile* file, OafStream* out_stream)
{
    if (out_stream == NULL)
    {
        return;
    }

    oaf_stream_init(
        out_stream,
        file,
        file_read_proc,
        file_write_proc,
        file_seek_proc,
        file_tell_proc,
        file_flush_proc,
        file_close_proc);
}
