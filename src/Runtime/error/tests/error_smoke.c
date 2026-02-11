#include <stdio.h>
#include <string.h>
#include "runtime.h"
#include "error.h"

static int test_stack_trace_collection(void)
{
    OafStackTrace trace;
    OafSourceLocation first_location;
    OafSourceLocation second_location;
    char buffer[512];

    oaf_stack_trace_init(&trace);

    first_location.file_name = "trace.oaf";
    first_location.line = 10;
    first_location.column = 2;
    second_location.file_name = "trace.oaf";
    second_location.line = 11;
    second_location.column = 4;

    if (!oaf_stack_trace_push(&trace, "outer", first_location)
        || !oaf_stack_trace_push(&trace, "inner", second_location))
    {
        return 0;
    }

    if (oaf_stack_trace_depth(&trace) != 2)
    {
        return 0;
    }

    if (!oaf_stack_trace_format(&trace, buffer, sizeof(buffer)))
    {
        return 0;
    }

    if (strstr(buffer, "outer") == NULL || strstr(buffer, "inner") == NULL)
    {
        return 0;
    }

    if (!oaf_stack_trace_pop(&trace) || oaf_stack_trace_depth(&trace) != 1)
    {
        return 0;
    }

    return 1;
}

static int test_error_propagation_chain(void)
{
    OafRuntimeError root;
    OafRuntimeError wrapper;
    OafSourceLocation location;
    char buffer[512];

    location.file_name = "error_chain.oaf";
    location.line = 21;
    location.column = 9;

    oaf_runtime_error_init(&root, "RootError", "root cause", location, NULL);
    oaf_runtime_error_wrap(&wrapper, "WrapperError", "wrapped", location, &root);

    if (oaf_runtime_error_chain_depth(&wrapper) != 2)
    {
        return 0;
    }

    if (oaf_runtime_error_root_cause(&wrapper) != &root)
    {
        return 0;
    }

    if (!oaf_runtime_error_format(&wrapper, buffer, sizeof(buffer)))
    {
        return 0;
    }

    return strstr(buffer, "caused by") != NULL;
}

typedef struct RecoverTestState
{
    OafRuntimeError error;
    int recovered;
} RecoverTestState;

static int failing_operation(OafContext* context, void* state_ptr)
{
    RecoverTestState* state = (RecoverTestState*)state_ptr;

    oaf_context_set_source_location(context, "recover.oaf", 30, 7);
    oaf_stack_trace_push(context->stack_trace, "failing_operation", context->caller_location);
    oaf_runtime_error_init(&state->error, "RecoverableError", "something failed", context->caller_location, NULL);
    oaf_context_report_error(context, &state->error);
    return 0;
}

static int recover_operation(OafContext* context, const OafRuntimeError* error, void* state_ptr)
{
    RecoverTestState* state = (RecoverTestState*)state_ptr;
    state->recovered = error != NULL
        && strcmp(oaf_runtime_error_name(error), "RecoverableError") == 0
        && error->stack_trace != NULL
        && oaf_stack_trace_depth(error->stack_trace) > 0;

    if (context->stack_trace != NULL)
    {
        oaf_stack_trace_init(context->stack_trace);
    }

    return state->recovered;
}

static int test_error_recovery(void)
{
    OafRuntime runtime = {0};
    OafRuntimeOptions options;
    RecoverTestState state;
    OafContext* context;
    int recovered;

    oaf_runtime_options_default(&options);
    if (oaf_runtime_init(&runtime, &options) != OAF_RUNTIME_STATUS_OK)
    {
        return 0;
    }

    state.recovered = 0;
    context = oaf_runtime_context(&runtime);
    if (context == NULL)
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    recovered = oaf_error_try_recover(context, failing_operation, &state, recover_operation, &state);
    if (!recovered || !state.recovered || oaf_context_has_error(context))
    {
        oaf_runtime_shutdown(&runtime);
        return 0;
    }

    oaf_runtime_shutdown(&runtime);
    return 1;
}

int main(void)
{
    int ok = 1;
    ok = ok && test_stack_trace_collection();
    ok = ok && test_error_propagation_chain();
    ok = ok && test_error_recovery();

    if (!ok)
    {
        fprintf(stderr, "error smoke tests failed\n");
        return 1;
    }

    printf("error smoke tests passed\n");
    return 0;
}
