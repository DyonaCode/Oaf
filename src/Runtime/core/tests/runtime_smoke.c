#include <stdio.h>
#include <string.h>
#include "runtime.h"

typedef struct ErrorHandlerState
{
    int invocation_count;
    int saw_expected_name;
} ErrorHandlerState;

static int capture_error(const OafRuntimeError* error, void* state_ptr)
{
    ErrorHandlerState* state = (ErrorHandlerState*)state_ptr;

    state->invocation_count++;
    state->saw_expected_name = strcmp(oaf_runtime_error_name(error), "ValidationError") == 0;
    return 1;
}

static int test_runtime_initialization(void)
{
    OafRuntime runtime = {0};
    OafRuntimeOptions options;
    OafAllocator temp_allocator;
    OafTypeRegistry* type_registry;
    OafThreadScheduler* scheduler;
    OafGarbageCollector* gc;
    OafSourceLocation location;

    oaf_runtime_options_default(&options);
    options.temp_allocator_capacity = 512;
    options.gc_enabled = 1;

    if (oaf_runtime_init(&runtime, &options) != OAF_RUNTIME_STATUS_OK)
    {
        return 0;
    }

    OafContext* context = oaf_runtime_context(&runtime);
    if (context == NULL || context->allocator == NULL || context->temp_allocator == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    if (context->stack_trace == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    if (context->gc_enabled == 0)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    scheduler = oaf_runtime_scheduler(&runtime);
    if (scheduler == NULL || scheduler->worker_count == 0 || context->scheduler != scheduler)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    gc = oaf_runtime_gc(&runtime);
    if (gc == NULL || !gc->enabled)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    type_registry = oaf_runtime_type_registry(&runtime);
    if (type_registry == NULL || oaf_type_registry_find_by_kind(type_registry, OAF_TYPE_KIND_INT) == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    location.file_name = "runtime_smoke.oaf";
    location.line = 5;
    location.column = 3;
    if (!oaf_stack_trace_push(context->stack_trace, "main", location))
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }
    oaf_stack_trace_pop(context->stack_trace);

    void* ptr = oaf_allocator_alloc(context->allocator, 32, 8);
    if (ptr == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }
    oaf_allocator_free(context->allocator, ptr);

    oaf_temp_allocator_as_allocator(context->temp_allocator, &temp_allocator);
    size_t mark = oaf_temp_allocator_mark(context->temp_allocator);
    if (mark == (size_t)-1)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    void* temp_ptr = oaf_allocator_alloc(&temp_allocator, 64, 8);
    if (temp_ptr == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    if (!oaf_temp_allocator_reset_to_mark(context->temp_allocator, mark))
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    oaf_runtime_shutdown(&runtime);
    return runtime.initialized == 0;
}

static int test_runtime_error_handling(void)
{
    OafRuntime runtime = {0};
    OafRuntimeOptions options;
    ErrorHandlerState state = {0};

    oaf_runtime_options_default(&options);
    options.temp_allocator_capacity = 512;

    if (oaf_runtime_init(&runtime, &options) != OAF_RUNTIME_STATUS_OK)
    {
        return 0;
    }

    OafContext* context = oaf_runtime_context(&runtime);
    if (context == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    oaf_context_set_error_handler(context, capture_error, &state);
    oaf_context_set_source_location(context, "runtime_smoke.oaf", 14, 6);

    OafRuntimeError error;
    oaf_runtime_error_init(
        &error,
        "ValidationError",
        "Invalid runtime state.",
        context->caller_location,
        NULL);

    if (!oaf_context_report_error(context, &error))
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    const OafRuntimeError* last_error = oaf_runtime_last_error(&runtime);
    int ok = (state.invocation_count == 1)
        && (state.saw_expected_name != 0)
        && (last_error == &error)
        && (last_error->location.line == 14)
        && (last_error->location.column == 6);

    oaf_runtime_shutdown(&runtime);
    return ok;
}

static int test_runtime_guards(void)
{
    OafRuntime runtime = {0};

    if (oaf_runtime_init(NULL, NULL) != OAF_RUNTIME_STATUS_INVALID_ARGUMENT)
    {
        return 0;
    }

    if (oaf_runtime_context(&runtime) != NULL)
    {
        return 0;
    }

    return 1;
}

int main(void)
{
    int ok = 1;
    ok = ok && test_runtime_initialization();
    ok = ok && test_runtime_error_handling();
    ok = ok && test_runtime_guards();

    if (!ok)
    {
        fprintf(stderr, "runtime smoke tests failed\n");
        return 1;
    }

    printf("runtime smoke tests passed\n");
    return 0;
}
