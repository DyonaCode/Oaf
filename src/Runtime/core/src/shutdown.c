#include "runtime.h"

void oaf_runtime_shutdown(OafRuntime* runtime)
{
    if (runtime == NULL)
    {
        return;
    }

    if (runtime->initialized != 0)
    {
        oaf_temp_allocator_destroy(&runtime->temp_allocator_state);
        oaf_gc_destroy(&runtime->gc);
        oaf_scheduler_shutdown(&runtime->scheduler);
        runtime->initialized = 0;
    }

    oaf_context_reset(&runtime->context);
    runtime->context.allocator = NULL;
    runtime->context.temp_allocator = NULL;
    runtime->context.stack_trace = NULL;
    oaf_type_registry_init(&runtime->type_registry);
    oaf_stack_trace_init(&runtime->stack_trace);
    oaf_runtime_error_clear(&runtime->startup_error);
}
