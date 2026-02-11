#ifndef OAF_TYPE_INFO_H
#define OAF_TYPE_INFO_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_MAX_REGISTERED_TYPES 256

typedef enum OafTypeKind
{
    OAF_TYPE_KIND_UNKNOWN = 0,
    OAF_TYPE_KIND_VOID = 1,
    OAF_TYPE_KIND_BOOL = 2,
    OAF_TYPE_KIND_CHAR = 3,
    OAF_TYPE_KIND_INT = 4,
    OAF_TYPE_KIND_FLOAT = 5,
    OAF_TYPE_KIND_STRING = 6,
    OAF_TYPE_KIND_STRUCT = 7,
    OAF_TYPE_KIND_INTERFACE = 8
} OafTypeKind;

struct OafTypeInfo;

typedef struct OafFieldInfo
{
    const char* name;
    const struct OafTypeInfo* type;
    size_t offset;
} OafFieldInfo;

typedef struct OafMethodInfo
{
    const char* name;
    const void* function;
} OafMethodInfo;

typedef struct OafInterfaceInfo
{
    const char* name;
    const OafMethodInfo* methods;
    size_t method_count;
} OafInterfaceInfo;

typedef struct OafTypeInfo
{
    OafTypeKind kind;
    const char* name;
    size_t size;
    size_t alignment;
    const struct OafTypeInfo* base;
    const OafFieldInfo* fields;
    size_t field_count;
    const OafMethodInfo* methods;
    size_t method_count;
    const OafInterfaceInfo* interfaces;
    size_t interface_count;
} OafTypeInfo;

typedef struct OafTypeRegistry
{
    const OafTypeInfo* entries[OAF_MAX_REGISTERED_TYPES];
    size_t count;
} OafTypeRegistry;

void oaf_type_registry_init(OafTypeRegistry* registry);
int oaf_type_registry_register(OafTypeRegistry* registry, const OafTypeInfo* type);
const OafTypeInfo* oaf_type_registry_find_by_name(const OafTypeRegistry* registry, const char* name);
const OafTypeInfo* oaf_type_registry_find_by_kind(const OafTypeRegistry* registry, OafTypeKind kind);
int oaf_type_registry_register_builtins(OafTypeRegistry* registry);
const OafTypeInfo* oaf_builtin_type_info(OafTypeKind kind);

const OafInterfaceInfo* oaf_type_find_interface(const OafTypeInfo* type, const char* interface_name);
int oaf_type_implements_interface(const OafTypeInfo* type, const OafInterfaceInfo* interface_info);
const OafMethodInfo* oaf_type_find_method(const OafTypeInfo* type, const char* method_name);

#ifdef __cplusplus
}
#endif

#endif
