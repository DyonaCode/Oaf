#include <string.h>
#include "oaf_serializer.h"

static int ensure_capacity(OafByteBuffer* buffer, size_t additional_bytes)
{
    size_t required;
    size_t next_capacity;
    uint8_t* resized;

    if (buffer == NULL || buffer->allocator == NULL)
    {
        return 0;
    }

    if (buffer->length > (SIZE_MAX - additional_bytes))
    {
        return 0;
    }

    required = buffer->length + additional_bytes;
    if (required <= buffer->capacity)
    {
        return 1;
    }

    next_capacity = buffer->capacity == 0 ? 64u : buffer->capacity;
    while (next_capacity < required)
    {
        if (next_capacity > (SIZE_MAX / 2u))
        {
            next_capacity = required;
            break;
        }

        next_capacity *= 2u;
    }

    if (buffer->data == NULL)
    {
        resized = (uint8_t*)oaf_allocator_alloc(buffer->allocator, next_capacity, _Alignof(uint8_t));
    }
    else
    {
        resized = (uint8_t*)oaf_allocator_realloc(
            buffer->allocator,
            buffer->data,
            buffer->capacity,
            next_capacity,
            _Alignof(uint8_t));
    }

    if (resized == NULL)
    {
        return 0;
    }

    buffer->data = resized;
    buffer->capacity = next_capacity;
    return 1;
}

int oaf_buffer_init(OafByteBuffer* buffer, OafAllocator* allocator)
{
    if (buffer == NULL || allocator == NULL)
    {
        return 0;
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
    buffer->allocator = allocator;
    return 1;
}

void oaf_buffer_destroy(OafByteBuffer* buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    if (buffer->data != NULL && buffer->allocator != NULL)
    {
        oaf_allocator_free(buffer->allocator, buffer->data);
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
    buffer->allocator = NULL;
}

void oaf_buffer_clear(OafByteBuffer* buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    buffer->length = 0;
}

int oaf_buffer_reserve(OafByteBuffer* buffer, size_t min_capacity)
{
    if (buffer == NULL)
    {
        return 0;
    }

    if (buffer->capacity >= min_capacity)
    {
        return 1;
    }

    return ensure_capacity(buffer, min_capacity - buffer->length);
}

int oaf_buffer_write_bytes(OafByteBuffer* buffer, const void* data, size_t length)
{
    if (buffer == NULL || data == NULL)
    {
        return 0;
    }

    if (!ensure_capacity(buffer, length))
    {
        return 0;
    }

    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;
    return 1;
}

int oaf_buffer_write_u8(OafByteBuffer* buffer, uint8_t value)
{
    return oaf_buffer_write_bytes(buffer, &value, sizeof(uint8_t));
}

int oaf_buffer_write_u32(OafByteBuffer* buffer, uint32_t value)
{
    uint8_t encoded[4];
    encoded[0] = (uint8_t)(value & 0xFFu);
    encoded[1] = (uint8_t)((value >> 8) & 0xFFu);
    encoded[2] = (uint8_t)((value >> 16) & 0xFFu);
    encoded[3] = (uint8_t)((value >> 24) & 0xFFu);
    return oaf_buffer_write_bytes(buffer, encoded, sizeof(encoded));
}

int oaf_buffer_write_i64(OafByteBuffer* buffer, int64_t value)
{
    uint64_t normalized = (uint64_t)value;
    uint8_t encoded[8];
    size_t index;

    for (index = 0; index < sizeof(encoded); index++)
    {
        encoded[index] = (uint8_t)((normalized >> (index * 8u)) & 0xFFu);
    }

    return oaf_buffer_write_bytes(buffer, encoded, sizeof(encoded));
}

int oaf_buffer_write_f64(OafByteBuffer* buffer, double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return oaf_buffer_write_i64(buffer, (int64_t)bits);
}

int oaf_buffer_write_string(OafByteBuffer* buffer, const char* text, size_t length)
{
    if (buffer == NULL || text == NULL)
    {
        return 0;
    }

    if (length > UINT32_MAX)
    {
        return 0;
    }

    if (!oaf_buffer_write_u32(buffer, (uint32_t)length))
    {
        return 0;
    }

    return oaf_buffer_write_bytes(buffer, text, length);
}

void oaf_reader_init(OafByteReader* reader, const uint8_t* data, size_t length)
{
    if (reader == NULL)
    {
        return;
    }

    reader->data = data;
    reader->length = length;
    reader->offset = 0;
}

int oaf_reader_read_bytes(OafByteReader* reader, void* out_data, size_t length)
{
    if (reader == NULL || out_data == NULL)
    {
        return 0;
    }

    if (reader->offset > reader->length || length > (reader->length - reader->offset))
    {
        return 0;
    }

    memcpy(out_data, reader->data + reader->offset, length);
    reader->offset += length;
    return 1;
}

int oaf_reader_read_u8(OafByteReader* reader, uint8_t* out_value)
{
    if (out_value == NULL)
    {
        return 0;
    }

    return oaf_reader_read_bytes(reader, out_value, sizeof(uint8_t));
}

int oaf_reader_read_u32(OafByteReader* reader, uint32_t* out_value)
{
    uint8_t encoded[4];

    if (out_value == NULL || !oaf_reader_read_bytes(reader, encoded, sizeof(encoded)))
    {
        return 0;
    }

    *out_value = ((uint32_t)encoded[0])
        | (((uint32_t)encoded[1]) << 8u)
        | (((uint32_t)encoded[2]) << 16u)
        | (((uint32_t)encoded[3]) << 24u);
    return 1;
}

int oaf_reader_read_i64(OafByteReader* reader, int64_t* out_value)
{
    uint8_t encoded[8];
    uint64_t normalized = 0;
    size_t index;

    if (out_value == NULL || !oaf_reader_read_bytes(reader, encoded, sizeof(encoded)))
    {
        return 0;
    }

    for (index = 0; index < sizeof(encoded); index++)
    {
        normalized |= ((uint64_t)encoded[index]) << (index * 8u);
    }

    *out_value = (int64_t)normalized;
    return 1;
}

int oaf_reader_read_f64(OafByteReader* reader, double* out_value)
{
    int64_t bits;

    if (out_value == NULL || !oaf_reader_read_i64(reader, &bits))
    {
        return 0;
    }

    memcpy(out_value, &bits, sizeof(double));
    return 1;
}

int oaf_reader_read_string(OafByteReader* reader, const uint8_t** out_data, size_t* out_length)
{
    uint32_t length;

    if (reader == NULL || out_data == NULL || out_length == NULL)
    {
        return 0;
    }

    if (!oaf_reader_read_u32(reader, &length))
    {
        return 0;
    }

    if (reader->offset > reader->length || (size_t)length > (reader->length - reader->offset))
    {
        return 0;
    }

    *out_data = reader->data + reader->offset;
    *out_length = (size_t)length;
    reader->offset += (size_t)length;
    return 1;
}
