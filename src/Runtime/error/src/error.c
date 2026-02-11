#include <stddef.h>
#include "error.h"
#include "context.h"

static void copy_text(char* destination, size_t capacity, const char* source)
{
    size_t index = 0;

    if (destination == NULL || capacity == 0)
    {
        return;
    }

    if (source == NULL)
    {
        destination[0] = '\0';
        return;
    }

    while (source[index] != '\0' && (index + 1) < capacity)
    {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}

static size_t append_text(char* buffer, size_t capacity, size_t offset, const char* text)
{
    size_t index = 0;

    if (buffer == NULL || capacity == 0 || text == NULL)
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

    while (length > 0 && (offset + 1) < capacity)
    {
        length--;
        buffer[offset] = digits[length];
        offset++;
    }

    if (capacity > 0)
    {
        buffer[offset < capacity ? offset : (capacity - 1)] = '\0';
    }

    return offset;
}

void oaf_runtime_error_clear(OafRuntimeError* error)
{
    if (error == NULL)
    {
        return;
    }

    error->name = "RuntimeError";
    error->message[0] = '\0';
    error->location.file_name = NULL;
    error->location.line = 0;
    error->location.column = 0;
    error->stack_trace = NULL;
    error->cause = NULL;
}

void oaf_runtime_error_init(
    OafRuntimeError* error,
    const char* name,
    const char* message,
    OafSourceLocation location,
    OafRuntimeError* cause)
{
    if (error == NULL)
    {
        return;
    }

    oaf_runtime_error_clear(error);
    error->name = (name == NULL || name[0] == '\0') ? "RuntimeError" : name;
    error->location = location;
    error->cause = cause;
    copy_text(error->message, OAF_RUNTIME_ERROR_MESSAGE_CAPACITY, message);
}

void oaf_runtime_error_wrap(
    OafRuntimeError* destination,
    const char* name,
    const char* message,
    OafSourceLocation location,
    OafRuntimeError* cause)
{
    oaf_runtime_error_init(destination, name, message, location, cause);
}

void oaf_runtime_error_attach_stack_trace(OafRuntimeError* error, const OafStackTrace* stack_trace)
{
    if (error == NULL)
    {
        return;
    }

    error->stack_trace = stack_trace;
}

void oaf_runtime_error_set_message(OafRuntimeError* error, const char* message)
{
    if (error == NULL)
    {
        return;
    }

    copy_text(error->message, OAF_RUNTIME_ERROR_MESSAGE_CAPACITY, message);
}

const char* oaf_runtime_error_name(const OafRuntimeError* error)
{
    if (error == NULL || error->name == NULL)
    {
        return "RuntimeError";
    }

    return error->name;
}

const char* oaf_runtime_error_message(const OafRuntimeError* error)
{
    if (error == NULL)
    {
        return "";
    }

    return error->message;
}

const OafRuntimeError* oaf_runtime_error_root_cause(const OafRuntimeError* error)
{
    const OafRuntimeError* cursor = error;

    while (cursor != NULL && cursor->cause != NULL)
    {
        cursor = cursor->cause;
    }

    return cursor;
}

size_t oaf_runtime_error_chain_depth(const OafRuntimeError* error)
{
    size_t depth = 0;
    const OafRuntimeError* cursor = error;

    while (cursor != NULL)
    {
        depth++;
        cursor = cursor->cause;
    }

    return depth;
}

int oaf_runtime_error_format(const OafRuntimeError* error, char* buffer, size_t capacity)
{
    size_t offset = 0;
    const OafRuntimeError* cursor = error;
    size_t chain_index = 0;

    if (buffer == NULL || capacity == 0)
    {
        return 0;
    }

    buffer[0] = '\0';

    if (error == NULL)
    {
        append_text(buffer, capacity, 0, "<no error>");
        return 1;
    }

    while (cursor != NULL)
    {
        const char* name = oaf_runtime_error_name(cursor);
        const char* message = oaf_runtime_error_message(cursor);
        const char* file_name = cursor->location.file_name == NULL ? "<unknown>" : cursor->location.file_name;

        if (chain_index > 0)
        {
            offset = append_text(buffer, capacity, offset, "\ncaused by: ");
        }

        offset = append_text(buffer, capacity, offset, name);
        offset = append_text(buffer, capacity, offset, ": ");
        offset = append_text(buffer, capacity, offset, message);
        offset = append_text(buffer, capacity, offset, " (");
        offset = append_text(buffer, capacity, offset, file_name);
        offset = append_text(buffer, capacity, offset, ":");
        offset = append_uint(buffer, capacity, offset, cursor->location.line);
        offset = append_text(buffer, capacity, offset, ":");
        offset = append_uint(buffer, capacity, offset, cursor->location.column);
        offset = append_text(buffer, capacity, offset, ")");

        chain_index++;
        cursor = cursor->cause;
    }

    if (error->stack_trace != NULL && oaf_stack_trace_depth(error->stack_trace) > 0)
    {
        char trace_buffer[1024];
        oaf_stack_trace_format(error->stack_trace, trace_buffer, sizeof(trace_buffer));
        offset = append_text(buffer, capacity, offset, "\nstack trace:\n");
        append_text(buffer, capacity, offset, trace_buffer);
    }

    return 1;
}

int oaf_error_try_recover(
    OafContext* context,
    OafTryProc try_proc,
    void* try_state,
    OafRecoverProc recover_proc,
    void* recover_state)
{
    OafRuntimeError* previous_error;
    int try_result;

    if (context == NULL || try_proc == NULL)
    {
        return 0;
    }

    previous_error = context->last_error;
    context->last_error = NULL;
    try_result = try_proc(context, try_state);
    if (try_result != 0)
    {
        return 1;
    }

    if (context->last_error == NULL)
    {
        context->last_error = previous_error;
        return 0;
    }

    if (recover_proc == NULL)
    {
        return 0;
    }

    if (recover_proc(context, context->last_error, recover_state) != 0)
    {
        context->last_error = NULL;
        return 1;
    }

    return 0;
}
