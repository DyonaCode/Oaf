#include <stddef.h>
#include "stack_trace.h"

static size_t append_text(char* buffer, size_t capacity, size_t offset, const char* text)
{
    size_t index = 0;

    if (buffer == NULL || capacity == 0)
    {
        return offset;
    }

    if (text == NULL)
    {
        return offset;
    }

    while (text[index] != '\0' && (offset + 1) < capacity)
    {
        buffer[offset] = text[index];
        offset++;
        index++;
    }

    buffer[offset] = '\0';
    return offset;
}

static size_t append_uint(char* buffer, size_t capacity, size_t offset, size_t value)
{
    char digits[32];
    size_t length = 0;

    if (value == 0)
    {
        return append_text(buffer, capacity, offset, "0");
    }

    while (value > 0 && length < sizeof(digits))
    {
        digits[length] = (char)('0' + (value % 10));
        value /= 10;
        length++;
    }

    while (length > 0)
    {
        length--;
        if ((offset + 1) >= capacity)
        {
            break;
        }

        buffer[offset] = digits[length];
        offset++;
    }

    if (capacity > 0)
    {
        buffer[offset < capacity ? offset : (capacity - 1)] = '\0';
    }

    return offset;
}

void oaf_stack_trace_init(OafStackTrace* trace)
{
    size_t index;

    if (trace == NULL)
    {
        return;
    }

    trace->depth = 0;
    trace->overflow_count = 0;

    for (index = 0; index < OAF_STACK_TRACE_MAX_FRAMES; index++)
    {
        trace->frames[index].function_name = NULL;
        trace->frames[index].location.file_name = NULL;
        trace->frames[index].location.line = 0;
        trace->frames[index].location.column = 0;
    }
}

int oaf_stack_trace_push(OafStackTrace* trace, const char* function_name, OafSourceLocation location)
{
    OafStackFrame* frame;

    if (trace == NULL)
    {
        return 0;
    }

    if (trace->depth >= OAF_STACK_TRACE_MAX_FRAMES)
    {
        trace->overflow_count++;
        return 0;
    }

    frame = &trace->frames[trace->depth];
    frame->function_name = function_name;
    frame->location = location;
    trace->depth++;
    return 1;
}

int oaf_stack_trace_pop(OafStackTrace* trace)
{
    OafStackFrame* frame;

    if (trace == NULL || trace->depth == 0)
    {
        return 0;
    }

    trace->depth--;
    frame = &trace->frames[trace->depth];
    frame->function_name = NULL;
    frame->location.file_name = NULL;
    frame->location.line = 0;
    frame->location.column = 0;
    return 1;
}

size_t oaf_stack_trace_depth(const OafStackTrace* trace)
{
    if (trace == NULL)
    {
        return 0;
    }

    return trace->depth;
}

const OafStackFrame* oaf_stack_trace_frame(const OafStackTrace* trace, size_t index)
{
    if (trace == NULL || index >= trace->depth)
    {
        return NULL;
    }

    return &trace->frames[index];
}

int oaf_stack_trace_format(const OafStackTrace* trace, char* buffer, size_t capacity)
{
    size_t index;
    size_t offset = 0;

    if (buffer == NULL || capacity == 0)
    {
        return 0;
    }

    buffer[0] = '\0';

    if (trace == NULL || trace->depth == 0)
    {
        append_text(buffer, capacity, offset, "<empty stack trace>");
        return 1;
    }

    for (index = 0; index < trace->depth; index++)
    {
        const OafStackFrame* frame = &trace->frames[index];
        const char* function_name = frame->function_name == NULL ? "<unknown>" : frame->function_name;
        const char* file_name = frame->location.file_name == NULL ? "<unknown>" : frame->location.file_name;

        offset = append_text(buffer, capacity, offset, "#");
        offset = append_uint(buffer, capacity, offset, index);
        offset = append_text(buffer, capacity, offset, " ");
        offset = append_text(buffer, capacity, offset, function_name);
        offset = append_text(buffer, capacity, offset, " (");
        offset = append_text(buffer, capacity, offset, file_name);
        offset = append_text(buffer, capacity, offset, ":");
        offset = append_uint(buffer, capacity, offset, frame->location.line);
        offset = append_text(buffer, capacity, offset, ":");
        offset = append_uint(buffer, capacity, offset, frame->location.column);
        offset = append_text(buffer, capacity, offset, ")");

        if ((index + 1) < trace->depth)
        {
            offset = append_text(buffer, capacity, offset, "\n");
        }
    }

    if (trace->overflow_count > 0)
    {
        offset = append_text(buffer, capacity, offset, "\n... truncated ");
        offset = append_uint(buffer, capacity, offset, trace->overflow_count);
        append_text(buffer, capacity, offset, " frame(s)");
    }

    return 1;
}
