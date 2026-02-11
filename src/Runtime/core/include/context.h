#ifndef OAF_CONTEXT_H
#define OAF_CONTEXT_H

#include "allocator.h"
#include "temp_allocator.h"
#include "source_location.h"
#include "stack_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OafRuntimeError;
typedef struct OafRuntimeError OafRuntimeError;

typedef int (*OafErrorHandlerProc)(const OafRuntimeError* error, void* state);

typedef struct OafErrorHandler
{
    OafErrorHandlerProc proc;
    void* state;
} OafErrorHandler;

typedef struct OafContext
{
    OafAllocator* allocator;
    OafTempAllocatorState* temp_allocator;
    void* scheduler;
    OafStackTrace* stack_trace;
    OafErrorHandler error_handler;
    OafSourceLocation caller_location;
    void* thread_local;
    int gc_enabled;
    OafRuntimeError* last_error;
} OafContext;

void oaf_context_init(OafContext* context, OafAllocator* allocator, OafTempAllocatorState* temp_allocator);
void oaf_context_reset(OafContext* context);
void oaf_context_set_source_location(OafContext* context, const char* file_name, uint32_t line, uint32_t column);
void oaf_context_set_error_handler(OafContext* context, OafErrorHandlerProc proc, void* state);
void oaf_context_set_stack_trace(OafContext* context, OafStackTrace* stack_trace);
int oaf_context_report_error(OafContext* context, OafRuntimeError* error);
int oaf_context_has_error(const OafContext* context);
void oaf_context_clear_error(OafContext* context);
void oaf_context_set_gc_enabled(OafContext* context, int enabled);

#ifdef __cplusplus
}
#endif

#endif
