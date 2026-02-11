#ifndef OAF_STDLIB_FILE_H
#define OAF_STDLIB_FILE_H

#include <stddef.h>
#include "allocator.h"
#include "oaf_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafFile
{
    void* handle;
} OafFile;

int oaf_file_open(OafFile* file, const char* path, const char* mode);
int oaf_file_is_open(const OafFile* file);
size_t oaf_file_read(OafFile* file, void* buffer, size_t bytes);
size_t oaf_file_write(OafFile* file, const void* buffer, size_t bytes);
int oaf_file_seek(OafFile* file, long offset, int origin);
long oaf_file_tell(OafFile* file);
int oaf_file_flush(OafFile* file);
int oaf_file_close(OafFile* file);

int oaf_file_read_all_text(
    const char* path,
    OafAllocator* allocator,
    char** out_text,
    size_t* out_length);
int oaf_file_write_all_text(const char* path, const char* text, size_t length);

void oaf_stream_from_file(OafFile* file, OafStream* out_stream);

#ifdef __cplusplus
}
#endif

#endif
