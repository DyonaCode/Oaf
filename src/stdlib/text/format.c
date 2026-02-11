#include <stdio.h>
#include <string.h>
#include "oaf_format.h"

static int append_formatted(OafString* output, const char* format, va_list args, int clear_first)
{
    va_list probe;
    int required_chars;
    size_t required_length;

    if (output == NULL || format == NULL)
    {
        return 0;
    }

    va_copy(probe, args);
    required_chars = vsnprintf(NULL, 0, format, probe);
    va_end(probe);
    if (required_chars < 0)
    {
        return 0;
    }

    required_length = (size_t)required_chars;
    if (clear_first)
    {
        oaf_string_clear(output);
    }

    if (!oaf_string_reserve(output, output->length + required_length + 1u))
    {
        return 0;
    }

    if ((size_t)vsnprintf(output->data + output->length, required_length + 1u, format, args) != required_length)
    {
        return 0;
    }

    output->length += required_length;
    return 1;
}

int oaf_format_append(OafString* output, const char* format, ...)
{
    va_list args;
    int ok;

    va_start(args, format);
    ok = oaf_format_append_v(output, format, args);
    va_end(args);
    return ok;
}

int oaf_format_append_v(OafString* output, const char* format, va_list args)
{
    va_list copy;
    int ok;

    va_copy(copy, args);
    ok = append_formatted(output, format, copy, 0);
    va_end(copy);
    return ok;
}

int oaf_format_assign(OafString* output, const char* format, ...)
{
    va_list args;
    int ok;

    va_start(args, format);
    ok = oaf_format_assign_v(output, format, args);
    va_end(args);
    return ok;
}

int oaf_format_assign_v(OafString* output, const char* format, va_list args)
{
    va_list copy;
    int ok;

    va_copy(copy, args);
    ok = append_formatted(output, format, copy, 1);
    va_end(copy);
    return ok;
}
