#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "oaf_string.h"

static int grow_for_append(OafString* string, size_t extra_chars)
{
    size_t required;
    size_t next_capacity;
    char* resized;

    if (string == NULL || string->allocator == NULL)
    {
        return 0;
    }

    if (string->length > (SIZE_MAX - extra_chars - 1u))
    {
        return 0;
    }

    required = string->length + extra_chars + 1u;
    if (required <= string->capacity)
    {
        return 1;
    }

    next_capacity = string->capacity == 0 ? 16u : string->capacity;
    while (next_capacity < required)
    {
        if (next_capacity > (SIZE_MAX / 2u))
        {
            next_capacity = required;
            break;
        }

        next_capacity *= 2u;
    }

    if (string->data == NULL)
    {
        resized = (char*)oaf_allocator_alloc(string->allocator, next_capacity, _Alignof(char));
    }
    else
    {
        resized = (char*)oaf_allocator_realloc(
            string->allocator,
            string->data,
            string->capacity,
            next_capacity,
            _Alignof(char));
    }

    if (resized == NULL)
    {
        return 0;
    }

    string->data = resized;
    string->capacity = next_capacity;
    return 1;
}

int oaf_string_init(OafString* string, OafAllocator* allocator)
{
    if (string == NULL || allocator == NULL)
    {
        return 0;
    }

    string->data = NULL;
    string->length = 0;
    string->capacity = 0;
    string->allocator = allocator;
    return oaf_string_append_char(string, '\0');
}

int oaf_string_init_from_cstr(OafString* string, const char* text, OafAllocator* allocator)
{
    if (!oaf_string_init(string, allocator))
    {
        return 0;
    }

    string->length = 0;
    if (string->data != NULL)
    {
        string->data[0] = '\0';
    }

    if (text == NULL)
    {
        return 1;
    }

    return oaf_string_append_cstr(string, text);
}

void oaf_string_destroy(OafString* string)
{
    if (string == NULL)
    {
        return;
    }

    if (string->data != NULL && string->allocator != NULL)
    {
        oaf_allocator_free(string->allocator, string->data);
    }

    string->data = NULL;
    string->length = 0;
    string->capacity = 0;
    string->allocator = NULL;
}

int oaf_string_reserve(OafString* string, size_t min_capacity)
{
    if (string == NULL)
    {
        return 0;
    }

    if (string->capacity >= min_capacity)
    {
        return 1;
    }

    if (string->length > 0)
    {
        return grow_for_append(string, min_capacity - string->length - 1u);
    }

    if (string->capacity == 0 && string->data == NULL)
    {
        size_t target = min_capacity == 0 ? 1u : min_capacity;
        char* allocated = (char*)oaf_allocator_alloc(string->allocator, target, _Alignof(char));
        if (allocated == NULL)
        {
            return 0;
        }

        string->data = allocated;
        string->capacity = target;
        string->data[0] = '\0';
        return 1;
    }

    return grow_for_append(string, min_capacity - string->length - 1u);
}

void oaf_string_clear(OafString* string)
{
    if (string == NULL || string->data == NULL)
    {
        return;
    }

    string->length = 0;
    string->data[0] = '\0';
}

int oaf_string_append_n(OafString* string, const char* text, size_t length)
{
    if (string == NULL || text == NULL)
    {
        return 0;
    }

    if (!grow_for_append(string, length))
    {
        return 0;
    }

    memcpy(string->data + string->length, text, length);
    string->length += length;
    string->data[string->length] = '\0';
    return 1;
}

int oaf_string_append_cstr(OafString* string, const char* text)
{
    if (text == NULL)
    {
        return 0;
    }

    return oaf_string_append_n(string, text, strlen(text));
}

int oaf_string_append_char(OafString* string, char value)
{
    if (string == NULL)
    {
        return 0;
    }

    if (!grow_for_append(string, 1u))
    {
        return 0;
    }

    if (value != '\0')
    {
        string->data[string->length] = value;
        string->length++;
    }

    string->data[string->length] = '\0';
    return 1;
}

int oaf_string_equals_cstr(const OafString* string, const char* text)
{
    size_t length;

    if (string == NULL || string->data == NULL || text == NULL)
    {
        return 0;
    }

    length = strlen(text);
    if (length != string->length)
    {
        return 0;
    }

    return memcmp(string->data, text, length) == 0;
}

int oaf_string_starts_with(const OafString* string, const char* prefix)
{
    size_t length;

    if (string == NULL || string->data == NULL || prefix == NULL)
    {
        return 0;
    }

    length = strlen(prefix);
    if (length > string->length)
    {
        return 0;
    }

    return memcmp(string->data, prefix, length) == 0;
}

int oaf_string_ends_with(const OafString* string, const char* suffix)
{
    size_t length;

    if (string == NULL || string->data == NULL || suffix == NULL)
    {
        return 0;
    }

    length = strlen(suffix);
    if (length > string->length)
    {
        return 0;
    }

    return memcmp(string->data + (string->length - length), suffix, length) == 0;
}

void oaf_string_to_upper_ascii(OafString* string)
{
    size_t index;

    if (string == NULL || string->data == NULL)
    {
        return;
    }

    for (index = 0; index < string->length; index++)
    {
        string->data[index] = (char)toupper((unsigned char)string->data[index]);
    }
}

void oaf_string_to_lower_ascii(OafString* string)
{
    size_t index;

    if (string == NULL || string->data == NULL)
    {
        return;
    }

    for (index = 0; index < string->length; index++)
    {
        string->data[index] = (char)tolower((unsigned char)string->data[index]);
    }
}

void oaf_string_trim_ascii(OafString* string)
{
    size_t start;
    size_t end;
    size_t new_length;

    if (string == NULL || string->data == NULL || string->length == 0)
    {
        return;
    }

    start = 0;
    end = string->length;

    while (start < string->length && isspace((unsigned char)string->data[start]))
    {
        start++;
    }

    while (end > start && isspace((unsigned char)string->data[end - 1]))
    {
        end--;
    }

    new_length = end - start;
    if (start > 0 && new_length > 0)
    {
        memmove(string->data, string->data + start, new_length);
    }

    string->length = new_length;
    string->data[new_length] = '\0';
}
