#include <string.h>
#include "type_info.h"

static int text_equals(const char* left, const char* right)
{
    if (left == NULL || right == NULL)
    {
        return 0;
    }

    return strcmp(left, right) == 0;
}

static const OafTypeInfo BUILTIN_VOID = {
    OAF_TYPE_KIND_VOID, "void", 0u, 1u, NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

static const OafTypeInfo BUILTIN_BOOL = {
    OAF_TYPE_KIND_BOOL, "bool", sizeof(int), _Alignof(int), NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

static const OafTypeInfo BUILTIN_CHAR = {
    OAF_TYPE_KIND_CHAR, "char", sizeof(char), _Alignof(char), NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

static const OafTypeInfo BUILTIN_INT = {
    OAF_TYPE_KIND_INT, "int", sizeof(long long), _Alignof(long long), NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

static const OafTypeInfo BUILTIN_FLOAT = {
    OAF_TYPE_KIND_FLOAT, "float", sizeof(double), _Alignof(double), NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

static const OafTypeInfo BUILTIN_STRING = {
    OAF_TYPE_KIND_STRING, "string", sizeof(const char*), _Alignof(const char*), NULL, NULL, 0u, NULL, 0u, NULL, 0u
};

void oaf_type_registry_init(OafTypeRegistry* registry)
{
    size_t index;

    if (registry == NULL)
    {
        return;
    }

    registry->count = 0;
    for (index = 0; index < OAF_MAX_REGISTERED_TYPES; index++)
    {
        registry->entries[index] = NULL;
    }
}

int oaf_type_registry_register(OafTypeRegistry* registry, const OafTypeInfo* type)
{
    if (registry == NULL || type == NULL || type->name == NULL)
    {
        return 0;
    }

    if (registry->count >= OAF_MAX_REGISTERED_TYPES)
    {
        return 0;
    }

    if (oaf_type_registry_find_by_name(registry, type->name) != NULL)
    {
        return 0;
    }

    registry->entries[registry->count] = type;
    registry->count++;
    return 1;
}

const OafTypeInfo* oaf_type_registry_find_by_name(const OafTypeRegistry* registry, const char* name)
{
    size_t index;

    if (registry == NULL || name == NULL)
    {
        return NULL;
    }

    for (index = 0; index < registry->count; index++)
    {
        const OafTypeInfo* candidate = registry->entries[index];

        if (candidate != NULL && text_equals(candidate->name, name))
        {
            return candidate;
        }
    }

    return NULL;
}

const OafTypeInfo* oaf_type_registry_find_by_kind(const OafTypeRegistry* registry, OafTypeKind kind)
{
    size_t index;

    if (registry == NULL)
    {
        return NULL;
    }

    for (index = 0; index < registry->count; index++)
    {
        const OafTypeInfo* candidate = registry->entries[index];

        if (candidate != NULL && candidate->kind == kind)
        {
            return candidate;
        }
    }

    return NULL;
}

const OafTypeInfo* oaf_builtin_type_info(OafTypeKind kind)
{
    switch (kind)
    {
        case OAF_TYPE_KIND_VOID:
            return &BUILTIN_VOID;
        case OAF_TYPE_KIND_BOOL:
            return &BUILTIN_BOOL;
        case OAF_TYPE_KIND_CHAR:
            return &BUILTIN_CHAR;
        case OAF_TYPE_KIND_INT:
            return &BUILTIN_INT;
        case OAF_TYPE_KIND_FLOAT:
            return &BUILTIN_FLOAT;
        case OAF_TYPE_KIND_STRING:
            return &BUILTIN_STRING;
        default:
            return NULL;
    }
}

int oaf_type_registry_register_builtins(OafTypeRegistry* registry)
{
    OafTypeKind builtin_kinds[] = {
        OAF_TYPE_KIND_VOID,
        OAF_TYPE_KIND_BOOL,
        OAF_TYPE_KIND_CHAR,
        OAF_TYPE_KIND_INT,
        OAF_TYPE_KIND_FLOAT,
        OAF_TYPE_KIND_STRING
    };
    size_t index;

    if (registry == NULL)
    {
        return 0;
    }

    for (index = 0; index < (sizeof(builtin_kinds) / sizeof(builtin_kinds[0])); index++)
    {
        const OafTypeInfo* builtin = oaf_builtin_type_info(builtin_kinds[index]);
        if (builtin == NULL || !oaf_type_registry_register(registry, builtin))
        {
            return 0;
        }
    }

    return 1;
}

const OafInterfaceInfo* oaf_type_find_interface(const OafTypeInfo* type, const char* interface_name)
{
    size_t index;

    if (type == NULL || interface_name == NULL)
    {
        return NULL;
    }

    for (index = 0; index < type->interface_count; index++)
    {
        const OafInterfaceInfo* info = &type->interfaces[index];
        if (text_equals(info->name, interface_name))
        {
            return info;
        }
    }

    if (type->base != NULL)
    {
        return oaf_type_find_interface(type->base, interface_name);
    }

    return NULL;
}

int oaf_type_implements_interface(const OafTypeInfo* type, const OafInterfaceInfo* interface_info)
{
    const OafInterfaceInfo* found;

    if (type == NULL || interface_info == NULL || interface_info->name == NULL)
    {
        return 0;
    }

    found = oaf_type_find_interface(type, interface_info->name);
    return found != NULL;
}

const OafMethodInfo* oaf_type_find_method(const OafTypeInfo* type, const char* method_name)
{
    size_t index;

    if (type == NULL || method_name == NULL)
    {
        return NULL;
    }

    for (index = 0; index < type->method_count; index++)
    {
        const OafMethodInfo* method = &type->methods[index];

        if (text_equals(method->name, method_name))
        {
            return method;
        }
    }

    if (type->base != NULL)
    {
        return oaf_type_find_method(type->base, method_name);
    }

    return NULL;
}
