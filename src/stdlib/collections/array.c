#include <stdint.h>
#include <string.h>
#include "array.h"

static int byte_count_for_capacity(const OafArray* array, size_t capacity, size_t* out_byte_count)
{
    if (array == NULL || out_byte_count == NULL || array->element_size == 0)
    {
        return 0;
    }

    if (capacity > (SIZE_MAX / array->element_size))
    {
        return 0;
    }

    *out_byte_count = capacity * array->element_size;
    return 1;
}

static int ensure_capacity(OafArray* array, size_t min_capacity)
{
    size_t next_capacity;

    if (array == NULL)
    {
        return 0;
    }

    if (min_capacity <= array->capacity)
    {
        return 1;
    }

    next_capacity = array->capacity == 0 ? 4u : array->capacity;
    while (next_capacity < min_capacity)
    {
        if (next_capacity > (SIZE_MAX / 2u))
        {
            next_capacity = min_capacity;
            break;
        }

        next_capacity *= 2u;
    }

    return oaf_array_reserve(array, next_capacity);
}

int oaf_array_init(OafArray* array, size_t element_size, size_t initial_capacity, OafAllocator* allocator)
{
    if (array == NULL || allocator == NULL || element_size == 0)
    {
        return 0;
    }

    array->data = NULL;
    array->length = 0;
    array->capacity = 0;
    array->element_size = element_size;
    array->allocator = allocator;

    if (initial_capacity == 0)
    {
        return 1;
    }

    return oaf_array_reserve(array, initial_capacity);
}

void oaf_array_destroy(OafArray* array)
{
    if (array == NULL)
    {
        return;
    }

    if (array->data != NULL && array->allocator != NULL)
    {
        oaf_allocator_free(array->allocator, array->data);
    }

    array->data = NULL;
    array->length = 0;
    array->capacity = 0;
    array->element_size = 0;
    array->allocator = NULL;
}

int oaf_array_reserve(OafArray* array, size_t min_capacity)
{
    size_t old_byte_count;
    size_t new_byte_count;
    void* resized;

    if (array == NULL || array->allocator == NULL)
    {
        return 0;
    }

    if (min_capacity <= array->capacity)
    {
        return 1;
    }

    if (!byte_count_for_capacity(array, array->capacity, &old_byte_count)
        || !byte_count_for_capacity(array, min_capacity, &new_byte_count))
    {
        return 0;
    }

    if (array->data == NULL)
    {
        resized = oaf_allocator_alloc(array->allocator, new_byte_count, _Alignof(max_align_t));
    }
    else
    {
        resized = oaf_allocator_realloc(
            array->allocator,
            array->data,
            old_byte_count,
            new_byte_count,
            _Alignof(max_align_t));
    }

    if (resized == NULL)
    {
        return 0;
    }

    array->data = resized;
    array->capacity = min_capacity;
    return 1;
}

int oaf_array_resize(OafArray* array, size_t new_length)
{
    size_t old_length;
    size_t new_bytes;
    unsigned char* bytes;

    if (array == NULL)
    {
        return 0;
    }

    old_length = array->length;
    if (new_length > old_length && !ensure_capacity(array, new_length))
    {
        return 0;
    }

    array->length = new_length;
    if (new_length <= old_length)
    {
        return 1;
    }

    if (!byte_count_for_capacity(array, new_length - old_length, &new_bytes))
    {
        return 0;
    }

    bytes = (unsigned char*)array->data;
    memset(bytes + (old_length * array->element_size), 0, new_bytes);
    return 1;
}

void oaf_array_clear(OafArray* array)
{
    if (array == NULL)
    {
        return;
    }

    array->length = 0;
}

void* oaf_array_at(OafArray* array, size_t index)
{
    unsigned char* bytes;

    if (array == NULL || index >= array->length)
    {
        return NULL;
    }

    bytes = (unsigned char*)array->data;
    return bytes + (index * array->element_size);
}

const void* oaf_array_at_const(const OafArray* array, size_t index)
{
    const unsigned char* bytes;

    if (array == NULL || index >= array->length)
    {
        return NULL;
    }

    bytes = (const unsigned char*)array->data;
    return bytes + (index * array->element_size);
}

int oaf_array_get(const OafArray* array, size_t index, void* out_element)
{
    const void* ptr;

    if (array == NULL || out_element == NULL)
    {
        return 0;
    }

    ptr = oaf_array_at_const(array, index);
    if (ptr == NULL)
    {
        return 0;
    }

    memcpy(out_element, ptr, array->element_size);
    return 1;
}

int oaf_array_set(OafArray* array, size_t index, const void* element)
{
    void* ptr;

    if (array == NULL || element == NULL)
    {
        return 0;
    }

    ptr = oaf_array_at(array, index);
    if (ptr == NULL)
    {
        return 0;
    }

    memcpy(ptr, element, array->element_size);
    return 1;
}

int oaf_array_push(OafArray* array, const void* element)
{
    if (array == NULL || element == NULL)
    {
        return 0;
    }

    if (!ensure_capacity(array, array->length + 1))
    {
        return 0;
    }

    memcpy((unsigned char*)array->data + (array->length * array->element_size), element, array->element_size);
    array->length++;
    return 1;
}

int oaf_array_pop(OafArray* array, void* out_element)
{
    unsigned char* last_element;

    if (array == NULL || array->length == 0)
    {
        return 0;
    }

    array->length--;
    last_element = (unsigned char*)array->data + (array->length * array->element_size);

    if (out_element != NULL)
    {
        memcpy(out_element, last_element, array->element_size);
    }

    memset(last_element, 0, array->element_size);
    return 1;
}

int oaf_array_insert(OafArray* array, size_t index, const void* element)
{
    unsigned char* bytes;

    if (array == NULL || element == NULL || index > array->length)
    {
        return 0;
    }

    if (!ensure_capacity(array, array->length + 1))
    {
        return 0;
    }

    bytes = (unsigned char*)array->data;
    if (index < array->length)
    {
        memmove(
            bytes + ((index + 1) * array->element_size),
            bytes + (index * array->element_size),
            (array->length - index) * array->element_size);
    }

    memcpy(bytes + (index * array->element_size), element, array->element_size);
    array->length++;
    return 1;
}

int oaf_array_remove_at(OafArray* array, size_t index, void* out_element)
{
    unsigned char* bytes;
    unsigned char* current;
    unsigned char* last;

    if (array == NULL || index >= array->length)
    {
        return 0;
    }

    bytes = (unsigned char*)array->data;
    current = bytes + (index * array->element_size);
    if (out_element != NULL)
    {
        memcpy(out_element, current, array->element_size);
    }

    if (index + 1 < array->length)
    {
        memmove(
            current,
            current + array->element_size,
            (array->length - index - 1) * array->element_size);
    }

    array->length--;
    last = bytes + (array->length * array->element_size);
    memset(last, 0, array->element_size);
    return 1;
}
