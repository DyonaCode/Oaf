#ifndef OAF_STACK_TRACE_H
#define OAF_STACK_TRACE_H

#include <stddef.h>
#include "source_location.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_STACK_TRACE_MAX_FRAMES 64

typedef struct OafStackFrame
{
    const char* function_name;
    OafSourceLocation location;
} OafStackFrame;

typedef struct OafStackTrace
{
    OafStackFrame frames[OAF_STACK_TRACE_MAX_FRAMES];
    size_t depth;
    size_t overflow_count;
} OafStackTrace;

void oaf_stack_trace_init(OafStackTrace* trace);
int oaf_stack_trace_push(OafStackTrace* trace, const char* function_name, OafSourceLocation location);
int oaf_stack_trace_pop(OafStackTrace* trace);
size_t oaf_stack_trace_depth(const OafStackTrace* trace);
const OafStackFrame* oaf_stack_trace_frame(const OafStackTrace* trace, size_t index);
int oaf_stack_trace_format(const OafStackTrace* trace, char* buffer, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
