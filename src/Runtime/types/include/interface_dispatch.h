#ifndef OAFLANG_INTERFACE_DISPATCH_H
#define OAFLANG_INTERFACE_DISPATCH_H

#include <stddef.h>
#include "type_info.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_MAX_INTERFACE_BINDINGS 32

typedef struct OafInterfaceBinding
{
    const OafInterfaceInfo* interface_info;
    const OafMethodInfo* methods;
    size_t method_count;
} OafInterfaceBinding;

typedef struct OafInterfaceDispatchTable
{
    const OafTypeInfo* implementing_type;
    OafInterfaceBinding bindings[OAF_MAX_INTERFACE_BINDINGS];
    size_t binding_count;
} OafInterfaceDispatchTable;

void oaf_interface_dispatch_init(OafInterfaceDispatchTable* table, const OafTypeInfo* implementing_type);
int oaf_interface_dispatch_bind(
    OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const OafMethodInfo* implementation_methods,
    size_t implementation_method_count);
const OafMethodInfo* oaf_interface_dispatch_resolve(
    const OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const char* method_name);
const void* oaf_interface_dispatch_resolve_proc(
    const OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const char* method_name);

#ifdef __cplusplus
}
#endif

#endif
