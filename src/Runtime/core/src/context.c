#include "context.h"
#include "error.h"

void oaf_context_init(OafContext* context, OafAllocator* allocator, OafTempAllocatorState* temp_allocator)
{
    if (context == NULL)
    {
        return;
    }

    context->allocator = allocator;
    context->temp_allocator = temp_allocator;
    context->scheduler = NULL;
    context->stack_trace = NULL;
    context->error_handler.proc = NULL;
    context->error_handler.state = NULL;
    context->caller_location.file_name = NULL;
    context->caller_location.line = 0;
    context->caller_location.column = 0;
    context->thread_local = NULL;
    context->gc_enabled = 0;
    context->last_error = NULL;
}

void oaf_context_reset(OafContext* context)
{
    if (context == NULL)
    {
        return;
    }

    context->scheduler = NULL;
    context->stack_trace = NULL;
    context->error_handler.proc = NULL;
    context->error_handler.state = NULL;
    context->caller_location.file_name = NULL;
    context->caller_location.line = 0;
    context->caller_location.column = 0;
    context->thread_local = NULL;
    context->gc_enabled = 0;
    context->last_error = NULL;
}

void oaf_context_set_source_location(OafContext* context, const char* file_name, uint32_t line, uint32_t column)
{
    if (context == NULL)
    {
        return;
    }

    context->caller_location.file_name = file_name;
    context->caller_location.line = line;
    context->caller_location.column = column;
}

void oaf_context_set_error_handler(OafContext* context, OafErrorHandlerProc proc, void* state)
{
    if (context == NULL)
    {
        return;
    }

    context->error_handler.proc = proc;
    context->error_handler.state = state;
}

void oaf_context_set_stack_trace(OafContext* context, OafStackTrace* stack_trace)
{
    if (context == NULL)
    {
        return;
    }

    context->stack_trace = stack_trace;
}

int oaf_context_report_error(OafContext* context, OafRuntimeError* error)
{
    if (context == NULL || error == NULL)
    {
        return 0;
    }

    context->last_error = error;
    if (error->location.file_name == NULL)
    {
        error->location = context->caller_location;
    }

    if (context->stack_trace != NULL)
    {
        oaf_runtime_error_attach_stack_trace(error, context->stack_trace);
    }

    if (context->error_handler.proc == NULL)
    {
        return 0;
    }

    return context->error_handler.proc(error, context->error_handler.state) != 0;
}

int oaf_context_has_error(const OafContext* context)
{
    if (context == NULL)
    {
        return 0;
    }

    return context->last_error != NULL;
}

void oaf_context_clear_error(OafContext* context)
{
    if (context == NULL)
    {
        return;
    }

    context->last_error = NULL;
}

void oaf_context_set_gc_enabled(OafContext* context, int enabled)
{
    if (context == NULL)
    {
        return;
    }

    context->gc_enabled = enabled != 0;
}
