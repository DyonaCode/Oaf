#ifndef OAFLANG_ERROR_H
#define OAFLANG_ERROR_H

#include <stddef.h>
#include "source_location.h"
#include "stack_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OAF_RUNTIME_ERROR_MESSAGE_CAPACITY
#define OAF_RUNTIME_ERROR_MESSAGE_CAPACITY 256
#endif

typedef struct OafRuntimeError
{
    const char* name;
    char message[OAF_RUNTIME_ERROR_MESSAGE_CAPACITY];
    OafSourceLocation location;
    const OafStackTrace* stack_trace;
    struct OafRuntimeError* cause;
} OafRuntimeError;

struct OafContext;
typedef struct OafContext OafContext;

typedef int (*OafTryProc)(OafContext* context, void* state);
typedef int (*OafRecoverProc)(OafContext* context, const OafRuntimeError* error, void* state);

void oaf_runtime_error_clear(OafRuntimeError* error);
void oaf_runtime_error_init(
    OafRuntimeError* error,
    const char* name,
    const char* message,
    OafSourceLocation location,
    OafRuntimeError* cause);
void oaf_runtime_error_wrap(
    OafRuntimeError* destination,
    const char* name,
    const char* message,
    OafSourceLocation location,
    OafRuntimeError* cause);
void oaf_runtime_error_attach_stack_trace(OafRuntimeError* error, const OafStackTrace* stack_trace);
void oaf_runtime_error_set_message(OafRuntimeError* error, const char* message);
const char* oaf_runtime_error_name(const OafRuntimeError* error);
const char* oaf_runtime_error_message(const OafRuntimeError* error);
const OafRuntimeError* oaf_runtime_error_root_cause(const OafRuntimeError* error);
size_t oaf_runtime_error_chain_depth(const OafRuntimeError* error);
int oaf_runtime_error_format(const OafRuntimeError* error, char* buffer, size_t capacity);
int oaf_error_try_recover(
    OafContext* context,
    OafTryProc try_proc,
    void* try_state,
    OafRecoverProc recover_proc,
    void* recover_state);

#ifdef __cplusplus
}
#endif

#endif
