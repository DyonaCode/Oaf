#include <stddef.h>
#include "marshalling.h"

void oaf_ffi_value_clear(OafFfiValue* value)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_VOID;
    value->as.i64 = 0;
}

void oaf_ffi_value_set_bool(OafFfiValue* value, bool data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_BOOL;
    value->as.bool_value = data;
}

void oaf_ffi_value_set_i32(OafFfiValue* value, int32_t data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_I32;
    value->as.i32 = data;
}

void oaf_ffi_value_set_i64(OafFfiValue* value, int64_t data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_I64;
    value->as.i64 = data;
}

void oaf_ffi_value_set_f32(OafFfiValue* value, float data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_F32;
    value->as.f32 = data;
}

void oaf_ffi_value_set_f64(OafFfiValue* value, double data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_F64;
    value->as.f64 = data;
}

void oaf_ffi_value_set_pointer(OafFfiValue* value, void* data)
{
    if (value == NULL)
    {
        return;
    }

    value->type = OAF_FOREIGN_TYPE_POINTER;
    value->as.pointer = data;
}

bool oaf_ffi_value_as_bool(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return false;
    }

    switch (value->type)
    {
        case OAF_FOREIGN_TYPE_BOOL:
            return value->as.bool_value;
        case OAF_FOREIGN_TYPE_I32:
            return value->as.i32 != 0;
        case OAF_FOREIGN_TYPE_I64:
            return value->as.i64 != 0;
        case OAF_FOREIGN_TYPE_POINTER:
            return value->as.pointer != NULL;
        default:
            return false;
    }
}

int32_t oaf_ffi_value_as_i32(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return 0;
    }

    switch (value->type)
    {
        case OAF_FOREIGN_TYPE_I32:
            return value->as.i32;
        case OAF_FOREIGN_TYPE_I64:
            return (int32_t)value->as.i64;
        case OAF_FOREIGN_TYPE_BOOL:
            return value->as.bool_value ? 1 : 0;
        case OAF_FOREIGN_TYPE_F32:
            return (int32_t)value->as.f32;
        case OAF_FOREIGN_TYPE_F64:
            return (int32_t)value->as.f64;
        default:
            return 0;
    }
}

int64_t oaf_ffi_value_as_i64(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return 0;
    }

    switch (value->type)
    {
        case OAF_FOREIGN_TYPE_I32:
            return value->as.i32;
        case OAF_FOREIGN_TYPE_I64:
            return value->as.i64;
        case OAF_FOREIGN_TYPE_BOOL:
            return value->as.bool_value ? 1 : 0;
        case OAF_FOREIGN_TYPE_F32:
            return (int64_t)value->as.f32;
        case OAF_FOREIGN_TYPE_F64:
            return (int64_t)value->as.f64;
        case OAF_FOREIGN_TYPE_POINTER:
            return (int64_t)(intptr_t)value->as.pointer;
        default:
            return 0;
    }
}

float oaf_ffi_value_as_f32(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return 0.0f;
    }

    switch (value->type)
    {
        case OAF_FOREIGN_TYPE_F32:
            return value->as.f32;
        case OAF_FOREIGN_TYPE_F64:
            return (float)value->as.f64;
        case OAF_FOREIGN_TYPE_I32:
            return (float)value->as.i32;
        case OAF_FOREIGN_TYPE_I64:
            return (float)value->as.i64;
        default:
            return 0.0f;
    }
}

double oaf_ffi_value_as_f64(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return 0.0;
    }

    switch (value->type)
    {
        case OAF_FOREIGN_TYPE_F32:
            return value->as.f32;
        case OAF_FOREIGN_TYPE_F64:
            return value->as.f64;
        case OAF_FOREIGN_TYPE_I32:
            return (double)value->as.i32;
        case OAF_FOREIGN_TYPE_I64:
            return (double)value->as.i64;
        default:
            return 0.0;
    }
}

void* oaf_ffi_value_as_pointer(const OafFfiValue* value)
{
    if (value == NULL)
    {
        return NULL;
    }

    if (value->type == OAF_FOREIGN_TYPE_POINTER)
    {
        return value->as.pointer;
    }

    if (value->type == OAF_FOREIGN_TYPE_I64)
    {
        return (void*)(intptr_t)value->as.i64;
    }

    return NULL;
}

int oaf_marshal_from_host(const void* source, OafForeignTypeKind type, OafFfiValue* destination)
{
    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    switch (type)
    {
        case OAF_FOREIGN_TYPE_BOOL:
            oaf_ffi_value_set_bool(destination, *(const bool*)source);
            return 1;
        case OAF_FOREIGN_TYPE_I32:
            oaf_ffi_value_set_i32(destination, *(const int32_t*)source);
            return 1;
        case OAF_FOREIGN_TYPE_I64:
            oaf_ffi_value_set_i64(destination, *(const int64_t*)source);
            return 1;
        case OAF_FOREIGN_TYPE_F32:
            oaf_ffi_value_set_f32(destination, *(const float*)source);
            return 1;
        case OAF_FOREIGN_TYPE_F64:
            oaf_ffi_value_set_f64(destination, *(const double*)source);
            return 1;
        case OAF_FOREIGN_TYPE_POINTER:
            oaf_ffi_value_set_pointer(destination, *(void* const*)source);
            return 1;
        case OAF_FOREIGN_TYPE_VOID:
        default:
            oaf_ffi_value_clear(destination);
            return type == OAF_FOREIGN_TYPE_VOID;
    }
}

int oaf_unmarshal_to_host(const OafFfiValue* source, void* destination, OafForeignTypeKind type)
{
    if (source == NULL || destination == NULL)
    {
        return 0;
    }

    switch (type)
    {
        case OAF_FOREIGN_TYPE_BOOL:
            *(bool*)destination = oaf_ffi_value_as_bool(source);
            return 1;
        case OAF_FOREIGN_TYPE_I32:
            *(int32_t*)destination = oaf_ffi_value_as_i32(source);
            return 1;
        case OAF_FOREIGN_TYPE_I64:
            *(int64_t*)destination = oaf_ffi_value_as_i64(source);
            return 1;
        case OAF_FOREIGN_TYPE_F32:
            *(float*)destination = oaf_ffi_value_as_f32(source);
            return 1;
        case OAF_FOREIGN_TYPE_F64:
            *(double*)destination = oaf_ffi_value_as_f64(source);
            return 1;
        case OAF_FOREIGN_TYPE_POINTER:
            *(void**)destination = oaf_ffi_value_as_pointer(source);
            return 1;
        case OAF_FOREIGN_TYPE_VOID:
        default:
            return type == OAF_FOREIGN_TYPE_VOID;
    }
}
