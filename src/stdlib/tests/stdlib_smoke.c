#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "default_allocator.h"
#include "oaf_algorithms.h"
#include "oaf_file.h"
#include "oaf_stream.h"
#include "oaf_string.h"
#include "oaf_format.h"
#include "oaf_serializer.h"

static int compare_int32(const void* left, const void* right, void* state)
{
    int32_t l = *(const int32_t*)left;
    int32_t r = *(const int32_t*)right;
    (void)state;
    if (l < r)
    {
        return -1;
    }

    if (l > r)
    {
        return 1;
    }

    return 0;
}

static int is_even(const void* value, void* state)
{
    (void)state;
    return (*(const int32_t*)value % 2) == 0;
}

static int test_algorithms(void)
{
    int32_t numbers[] = { 9, 1, 7, 5, 3, 8, 2, 4, 6 };
    size_t index;
    int32_t needle = 7;
    size_t split;
    size_t i;

    oaf_alg_sort(numbers, sizeof(numbers) / sizeof(numbers[0]), sizeof(numbers[0]), compare_int32, NULL);
    for (i = 1; i < (sizeof(numbers) / sizeof(numbers[0])); i++)
    {
        if (numbers[i - 1] > numbers[i])
        {
            return 0;
        }
    }

    if (!oaf_alg_binary_search(
            numbers,
            sizeof(numbers) / sizeof(numbers[0]),
            sizeof(numbers[0]),
            &needle,
            compare_int32,
            NULL,
            &index))
    {
        return 0;
    }

    if (numbers[index] != 7)
    {
        return 0;
    }

    oaf_alg_reverse(numbers, sizeof(numbers) / sizeof(numbers[0]), sizeof(numbers[0]));
    if (numbers[0] != 9 || numbers[8] != 1)
    {
        return 0;
    }

    split = oaf_alg_partition(numbers, sizeof(numbers) / sizeof(numbers[0]), sizeof(numbers[0]), is_even, NULL);
    for (i = 0; i < split; i++)
    {
        if ((numbers[i] % 2) != 0)
        {
            return 0;
        }
    }
    for (i = split; i < (sizeof(numbers) / sizeof(numbers[0])); i++)
    {
        if ((numbers[i] % 2) == 0)
        {
            return 0;
        }
    }

    return 1;
}

static int test_io_and_stream(void)
{
    const char* path = "stdlib_smoke_io.tmp";
    const char* text = "hello stdlib io";
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafFile file;
    OafStream stream;
    char read_buffer[32];
    char* all_text = NULL;
    size_t all_text_length = 0;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);

    if (!oaf_file_open(&file, path, "wb+"))
    {
        return 0;
    }

    oaf_stream_from_file(&file, &stream);
    ok = ok && oaf_stream_write(&stream, text, strlen(text)) == strlen(text);
    ok = ok && oaf_stream_flush(&stream);
    ok = ok && oaf_stream_seek(&stream, 0, SEEK_SET);

    memset(read_buffer, 0, sizeof(read_buffer));
    ok = ok && oaf_stream_read(&stream, read_buffer, strlen(text)) == strlen(text);
    ok = ok && strcmp(read_buffer, text) == 0;
    ok = ok && oaf_stream_close(&stream);

    ok = ok && oaf_file_read_all_text(path, &allocator, &all_text, &all_text_length);
    ok = ok && all_text != NULL && all_text_length == strlen(text) && strcmp(all_text, text) == 0;

    if (all_text != NULL)
    {
        oaf_allocator_free(&allocator, all_text);
    }

    remove(path);
    return ok && state.active_allocations == 0;
}

static int test_string_and_format(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafString value;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_string_init_from_cstr(&value, "  Hello", &allocator))
    {
        return 0;
    }

    ok = ok && oaf_string_append_cstr(&value, " World  ");
    oaf_string_trim_ascii(&value);
    ok = ok && oaf_string_equals_cstr(&value, "Hello World");
    ok = ok && oaf_string_starts_with(&value, "Hello");
    ok = ok && oaf_string_ends_with(&value, "World");

    oaf_string_to_upper_ascii(&value);
    ok = ok && oaf_string_equals_cstr(&value, "HELLO WORLD");
    oaf_string_to_lower_ascii(&value);
    ok = ok && oaf_string_equals_cstr(&value, "hello world");

    ok = ok && oaf_format_assign(&value, "count=%d", 42);
    ok = ok && oaf_string_equals_cstr(&value, "count=42");
    ok = ok && oaf_format_append(&value, ", %s", "done");
    ok = ok && oaf_string_equals_cstr(&value, "count=42, done");

    oaf_string_destroy(&value);
    return ok && state.active_allocations == 0;
}

static int test_serialization(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafByteBuffer buffer;
    OafByteReader reader;
    uint8_t u8_value;
    uint32_t u32_value;
    int64_t i64_value;
    double f64_value;
    const uint8_t* text;
    size_t text_length;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_buffer_init(&buffer, &allocator))
    {
        return 0;
    }

    ok = ok && oaf_buffer_write_u8(&buffer, 0x7Au);
    ok = ok && oaf_buffer_write_u32(&buffer, 0xDEADBEEFu);
    ok = ok && oaf_buffer_write_i64(&buffer, -123456789ll);
    ok = ok && oaf_buffer_write_f64(&buffer, 3.5);
    ok = ok && oaf_buffer_write_string(&buffer, "alpha", 5u);

    oaf_reader_init(&reader, buffer.data, buffer.length);

    ok = ok && oaf_reader_read_u8(&reader, &u8_value) && u8_value == 0x7Au;
    ok = ok && oaf_reader_read_u32(&reader, &u32_value) && u32_value == 0xDEADBEEFu;
    ok = ok && oaf_reader_read_i64(&reader, &i64_value) && i64_value == -123456789ll;
    ok = ok && oaf_reader_read_f64(&reader, &f64_value) && f64_value == 3.5;
    ok = ok && oaf_reader_read_string(&reader, &text, &text_length);
    ok = ok && text_length == 5u && memcmp(text, "alpha", text_length) == 0;

    oaf_buffer_destroy(&buffer);
    return ok && state.active_allocations == 0;
}

int main(void)
{
    if (!test_algorithms() || !test_io_and_stream() || !test_string_and_format() || !test_serialization())
    {
        fprintf(stderr, "stdlib smoke tests failed\n");
        return 1;
    }

    printf("stdlib smoke tests passed\n");
    return 0;
}
