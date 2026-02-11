#ifndef OAFLANG_STDLIB_SERIALIZER_H
#define OAFLANG_STDLIB_SERIALIZER_H

#include <stddef.h>
#include <stdint.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafByteBuffer
{
    uint8_t* data;
    size_t length;
    size_t capacity;
    OafAllocator* allocator;
} OafByteBuffer;

typedef struct OafByteReader
{
    const uint8_t* data;
    size_t length;
    size_t offset;
} OafByteReader;

int oaf_buffer_init(OafByteBuffer* buffer, OafAllocator* allocator);
void oaf_buffer_destroy(OafByteBuffer* buffer);
void oaf_buffer_clear(OafByteBuffer* buffer);
int oaf_buffer_reserve(OafByteBuffer* buffer, size_t min_capacity);
int oaf_buffer_write_bytes(OafByteBuffer* buffer, const void* data, size_t length);
int oaf_buffer_write_u8(OafByteBuffer* buffer, uint8_t value);
int oaf_buffer_write_u32(OafByteBuffer* buffer, uint32_t value);
int oaf_buffer_write_i64(OafByteBuffer* buffer, int64_t value);
int oaf_buffer_write_f64(OafByteBuffer* buffer, double value);
int oaf_buffer_write_string(OafByteBuffer* buffer, const char* text, size_t length);

void oaf_reader_init(OafByteReader* reader, const uint8_t* data, size_t length);
int oaf_reader_read_bytes(OafByteReader* reader, void* out_data, size_t length);
int oaf_reader_read_u8(OafByteReader* reader, uint8_t* out_value);
int oaf_reader_read_u32(OafByteReader* reader, uint32_t* out_value);
int oaf_reader_read_i64(OafByteReader* reader, int64_t* out_value);
int oaf_reader_read_f64(OafByteReader* reader, double* out_value);
int oaf_reader_read_string(OafByteReader* reader, const uint8_t** out_data, size_t* out_length);

#ifdef __cplusplus
}
#endif

#endif
