#ifndef OAFLANG_FFI_RUNTIME_H
#define OAFLANG_FFI_RUNTIME_H

#include <stddef.h>
#include "marshalling.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_FFI_MAX_ARGS 8

typedef enum OafFfiCallStatus
{
    OAF_FFI_CALL_OK = 0,
    OAF_FFI_CALL_INVALID_ARGUMENT = 1,
    OAF_FFI_CALL_UNSUPPORTED_SIGNATURE = 2,
    OAF_FFI_CALL_SYMBOL_NOT_FOUND = 3,
    OAF_FFI_CALL_LIBRARY_ERROR = 4
} OafFfiCallStatus;

typedef struct OafFfiSignature
{
    OafForeignTypeKind return_type;
    OafForeignTypeKind argument_types[OAF_FFI_MAX_ARGS];
    size_t argument_count;
} OafFfiSignature;

typedef struct OafFfiLibrary
{
    void* handle;
    int owns_handle;
} OafFfiLibrary;

void oaf_ffi_signature_clear(OafFfiSignature* signature);
int oaf_ffi_signature_add_argument(OafFfiSignature* signature, OafForeignTypeKind argument_type);

int oaf_ffi_library_open(OafFfiLibrary* library, const char* path);
void oaf_ffi_library_close(OafFfiLibrary* library);
void* oaf_ffi_library_symbol(const OafFfiLibrary* library, const char* symbol_name);

OafFfiCallStatus oaf_ffi_call_address(
    const void* function_address,
    const OafFfiSignature* signature,
    const OafFfiValue* arguments,
    OafFfiValue* result);
OafFfiCallStatus oaf_ffi_call_symbol(
    const OafFfiLibrary* library,
    const char* symbol_name,
    const OafFfiSignature* signature,
    const OafFfiValue* arguments,
    OafFfiValue* result);

#ifdef __cplusplus
}
#endif

#endif
