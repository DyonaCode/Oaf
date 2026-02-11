#ifndef OAFLANG_CALLBACK_REGISTRY_H
#define OAFLANG_CALLBACK_REGISTRY_H

#include <stddef.h>
#include <stdint.h>
#include "marshalling.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_FFI_MAX_CALLBACKS 64
#define OAF_FFI_MAX_TRAMPOLINES 8

typedef int64_t (*OafFfiCallbackProc)(const OafFfiValue* args, size_t arg_count, void* user_data);
typedef int64_t (*OafFfiTrampolineI64I64)(int64_t);

typedef struct OafFfiCallbackEntry
{
    uint64_t id;
    OafFfiCallbackProc proc;
    void* user_data;
    int active;
} OafFfiCallbackEntry;

typedef struct OafFfiCallbackRegistry
{
    OafFfiCallbackEntry entries[OAF_FFI_MAX_CALLBACKS];
    uint64_t next_id;
} OafFfiCallbackRegistry;

void oaf_ffi_callback_registry_init(OafFfiCallbackRegistry* registry);
uint64_t oaf_ffi_callback_register(OafFfiCallbackRegistry* registry, OafFfiCallbackProc proc, void* user_data);
int oaf_ffi_callback_unregister(OafFfiCallbackRegistry* registry, uint64_t callback_id);
int64_t oaf_ffi_callback_invoke_i64(
    const OafFfiCallbackRegistry* registry,
    uint64_t callback_id,
    const OafFfiValue* args,
    size_t arg_count,
    int* success);
int oaf_ffi_callback_acquire_trampoline_i64_i64(
    OafFfiCallbackRegistry* registry,
    uint64_t callback_id,
    OafFfiTrampolineI64I64* trampoline_out);
void oaf_ffi_callback_release_trampoline(OafFfiTrampolineI64I64 trampoline);

#ifdef __cplusplus
}
#endif

#endif
