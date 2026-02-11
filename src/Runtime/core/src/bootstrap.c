#include "runtime.h"

static OafSourceLocation runtime_bootstrap_location(void)
{
    OafSourceLocation location;
    location.file_name = "runtime.bootstrap";
    location.line = 0;
    location.column = 0;
    return location;
}

void oaf_runtime_options_default(OafRuntimeOptions* options)
{
    if (options == NULL)
    {
        return;
    }

    options->temp_allocator_capacity = OAF_RUNTIME_DEFAULT_TEMP_CAPACITY;
    options->scheduler_worker_count = 4;
    options->gc_enabled = 0;
}

OafRuntimeStatus oaf_runtime_init(OafRuntime* runtime, const OafRuntimeOptions* options)
{
    OafRuntimeOptions effective_options;

    if (runtime == NULL)
    {
        return OAF_RUNTIME_STATUS_INVALID_ARGUMENT;
    }

    if (runtime->initialized != 0)
    {
        return OAF_RUNTIME_STATUS_ALREADY_INITIALIZED;
    }

    oaf_runtime_options_default(&effective_options);
    if (options != NULL)
    {
        effective_options = *options;
        if (effective_options.temp_allocator_capacity == 0)
        {
            effective_options.temp_allocator_capacity = OAF_RUNTIME_DEFAULT_TEMP_CAPACITY;
        }
        if (effective_options.scheduler_worker_count == 0)
        {
            effective_options.scheduler_worker_count = 4;
        }
    }

    oaf_default_allocator_init(&runtime->default_allocator_state, &runtime->default_allocator);
    oaf_context_init(&runtime->context, &runtime->default_allocator, NULL);
    oaf_stack_trace_init(&runtime->stack_trace);
    oaf_context_set_stack_trace(&runtime->context, &runtime->stack_trace);
    oaf_runtime_error_clear(&runtime->startup_error);

    if (!oaf_scheduler_init(&runtime->scheduler, effective_options.scheduler_worker_count))
    {
        oaf_runtime_error_init(
            &runtime->startup_error,
            "RuntimeInitializationError",
            "Failed to initialize thread scheduler.",
            runtime_bootstrap_location(),
            NULL);
        runtime->context.last_error = &runtime->startup_error;
        runtime->initialized = 0;
        return OAF_RUNTIME_STATUS_INIT_FAILED;
    }

    oaf_type_registry_init(&runtime->type_registry);
    runtime->context.scheduler = &runtime->scheduler;

    if (!oaf_gc_init(&runtime->gc, &runtime->default_allocator, effective_options.gc_enabled))
    {
        oaf_runtime_error_init(
            &runtime->startup_error,
            "RuntimeInitializationError",
            "Failed to initialize garbage collector.",
            runtime_bootstrap_location(),
            NULL);
        runtime->context.last_error = &runtime->startup_error;
        oaf_scheduler_shutdown(&runtime->scheduler);
        runtime->initialized = 0;
        return OAF_RUNTIME_STATUS_INIT_FAILED;
    }

    if (!oaf_temp_allocator_init(&runtime->temp_allocator_state, effective_options.temp_allocator_capacity))
    {
        oaf_runtime_error_init(
            &runtime->startup_error,
            "RuntimeInitializationError",
            "Failed to initialize temporary allocator.",
            runtime_bootstrap_location(),
            NULL);
        runtime->context.last_error = &runtime->startup_error;
        oaf_gc_destroy(&runtime->gc);
        oaf_scheduler_shutdown(&runtime->scheduler);
        runtime->initialized = 0;
        return OAF_RUNTIME_STATUS_INIT_FAILED;
    }

    runtime->context.temp_allocator = &runtime->temp_allocator_state;
    oaf_context_set_gc_enabled(&runtime->context, effective_options.gc_enabled);

    if (!oaf_type_registry_register_builtins(&runtime->type_registry))
    {
        oaf_runtime_error_init(
            &runtime->startup_error,
            "RuntimeInitializationError",
            "Failed to initialize runtime type registry.",
            runtime_bootstrap_location(),
            NULL);
        runtime->context.last_error = &runtime->startup_error;
        oaf_temp_allocator_destroy(&runtime->temp_allocator_state);
        oaf_gc_destroy(&runtime->gc);
        oaf_scheduler_shutdown(&runtime->scheduler);
        runtime->initialized = 0;
        return OAF_RUNTIME_STATUS_INIT_FAILED;
    }

    runtime->initialized = 1;
    return OAF_RUNTIME_STATUS_OK;
}

OafContext* oaf_runtime_context(OafRuntime* runtime)
{
    if (runtime == NULL || runtime->initialized == 0)
    {
        return NULL;
    }

    return &runtime->context;
}

const OafRuntimeError* oaf_runtime_last_error(const OafRuntime* runtime)
{
    if (runtime == NULL)
    {
        return NULL;
    }

    if (runtime->context.last_error != NULL)
    {
        return runtime->context.last_error;
    }

    if (runtime->startup_error.message[0] != '\0')
    {
        return &runtime->startup_error;
    }

    return NULL;
}

OafTypeRegistry* oaf_runtime_type_registry(OafRuntime* runtime)
{
    if (runtime == NULL || runtime->initialized == 0)
    {
        return NULL;
    }

    return &runtime->type_registry;
}

OafThreadScheduler* oaf_runtime_scheduler(OafRuntime* runtime)
{
    if (runtime == NULL || runtime->initialized == 0)
    {
        return NULL;
    }

    return &runtime->scheduler;
}

OafGarbageCollector* oaf_runtime_gc(OafRuntime* runtime)
{
    if (runtime == NULL || runtime->initialized == 0)
    {
        return NULL;
    }

    return &runtime->gc;
}
