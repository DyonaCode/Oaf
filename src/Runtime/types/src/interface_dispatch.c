#include <string.h>
#include "interface_dispatch.h"

static int text_equals(const char* left, const char* right)
{
    if (left == NULL || right == NULL)
    {
        return 0;
    }

    return strcmp(left, right) == 0;
}

static const OafMethodInfo* find_method_by_name(const OafMethodInfo* methods, size_t method_count, const char* method_name)
{
    size_t index;

    if (methods == NULL || method_name == NULL)
    {
        return NULL;
    }

    for (index = 0; index < method_count; index++)
    {
        const OafMethodInfo* method = &methods[index];
        if (text_equals(method->name, method_name))
        {
            return method;
        }
    }

    return NULL;
}

void oaf_interface_dispatch_init(OafInterfaceDispatchTable* table, const OafTypeInfo* implementing_type)
{
    size_t index;

    if (table == NULL)
    {
        return;
    }

    table->implementing_type = implementing_type;
    table->binding_count = 0;

    for (index = 0; index < OAF_MAX_INTERFACE_BINDINGS; index++)
    {
        table->bindings[index].interface_info = NULL;
        table->bindings[index].methods = NULL;
        table->bindings[index].method_count = 0;
    }
}

int oaf_interface_dispatch_bind(
    OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const OafMethodInfo* implementation_methods,
    size_t implementation_method_count)
{
    size_t binding_index;
    OafInterfaceBinding* binding;
    size_t required_index;

    if (table == NULL || table->implementing_type == NULL || interface_info == NULL)
    {
        return 0;
    }

    if (implementation_methods == NULL || implementation_method_count == 0)
    {
        return 0;
    }

    if (!oaf_type_implements_interface(table->implementing_type, interface_info))
    {
        return 0;
    }

    for (required_index = 0; required_index < interface_info->method_count; required_index++)
    {
        const OafMethodInfo* required_method = &interface_info->methods[required_index];
        const OafMethodInfo* provided = find_method_by_name(
            implementation_methods,
            implementation_method_count,
            required_method->name);

        if (provided == NULL || provided->function == NULL)
        {
            return 0;
        }
    }

    for (binding_index = 0; binding_index < table->binding_count; binding_index++)
    {
        if (table->bindings[binding_index].interface_info != NULL
            && text_equals(table->bindings[binding_index].interface_info->name, interface_info->name))
        {
            table->bindings[binding_index].methods = implementation_methods;
            table->bindings[binding_index].method_count = implementation_method_count;
            return 1;
        }
    }

    if (table->binding_count >= OAF_MAX_INTERFACE_BINDINGS)
    {
        return 0;
    }

    binding = &table->bindings[table->binding_count];
    binding->interface_info = interface_info;
    binding->methods = implementation_methods;
    binding->method_count = implementation_method_count;
    table->binding_count++;
    return 1;
}

const OafMethodInfo* oaf_interface_dispatch_resolve(
    const OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const char* method_name)
{
    size_t binding_index;

    if (table == NULL || interface_info == NULL || method_name == NULL)
    {
        return NULL;
    }

    for (binding_index = 0; binding_index < table->binding_count; binding_index++)
    {
        const OafInterfaceBinding* binding = &table->bindings[binding_index];

        if (binding->interface_info == NULL)
        {
            continue;
        }

        if (text_equals(binding->interface_info->name, interface_info->name))
        {
            return find_method_by_name(binding->methods, binding->method_count, method_name);
        }
    }

    return NULL;
}

const void* oaf_interface_dispatch_resolve_proc(
    const OafInterfaceDispatchTable* table,
    const OafInterfaceInfo* interface_info,
    const char* method_name)
{
    const OafMethodInfo* method = oaf_interface_dispatch_resolve(table, interface_info, method_name);

    if (method == NULL)
    {
        return NULL;
    }

    return method->function;
}
