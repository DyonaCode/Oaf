#include <string.h>
#include "reflection.h"

static int text_equals(const char* left, const char* right)
{
    if (left == NULL || right == NULL)
    {
        return 0;
    }

    return strcmp(left, right) == 0;
}

size_t oaf_reflection_field_count(const OafTypeInfo* type)
{
    if (type == NULL)
    {
        return 0;
    }

    return type->field_count;
}

const OafFieldInfo* oaf_reflection_field_at(const OafTypeInfo* type, size_t index)
{
    if (type == NULL || index >= type->field_count)
    {
        return NULL;
    }

    return &type->fields[index];
}

const OafFieldInfo* oaf_reflection_find_field(const OafTypeInfo* type, const char* field_name)
{
    size_t index;

    if (type == NULL || field_name == NULL)
    {
        return NULL;
    }

    for (index = 0; index < type->field_count; index++)
    {
        const OafFieldInfo* field = &type->fields[index];
        if (text_equals(field->name, field_name))
        {
            return field;
        }
    }

    if (type->base != NULL)
    {
        return oaf_reflection_find_field(type->base, field_name);
    }

    return NULL;
}

size_t oaf_reflection_method_count(const OafTypeInfo* type)
{
    if (type == NULL)
    {
        return 0;
    }

    return type->method_count;
}

const OafMethodInfo* oaf_reflection_method_at(const OafTypeInfo* type, size_t index)
{
    if (type == NULL || index >= type->method_count)
    {
        return NULL;
    }

    return &type->methods[index];
}

const OafMethodInfo* oaf_reflection_find_method(const OafTypeInfo* type, const char* method_name)
{
    return oaf_type_find_method(type, method_name);
}

size_t oaf_reflection_interface_count(const OafTypeInfo* type)
{
    if (type == NULL)
    {
        return 0;
    }

    return type->interface_count;
}

const OafInterfaceInfo* oaf_reflection_interface_at(const OafTypeInfo* type, size_t index)
{
    if (type == NULL || index >= type->interface_count)
    {
        return NULL;
    }

    return &type->interfaces[index];
}

const OafInterfaceInfo* oaf_reflection_find_interface(const OafTypeInfo* type, const char* interface_name)
{
    return oaf_type_find_interface(type, interface_name);
}

int oaf_reflection_implements_interface(const OafTypeInfo* type, const OafInterfaceInfo* interface_info)
{
    return oaf_type_implements_interface(type, interface_info);
}
