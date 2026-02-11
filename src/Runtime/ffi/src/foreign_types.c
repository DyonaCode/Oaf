#include <stdbool.h>
#include <stdint.h>
#include "foreign_types.h"

static const OafForeignTypeDescriptor FOREIGN_TYPE_DESCRIPTORS[] = {
    { OAF_FOREIGN_TYPE_VOID, "void", 0u, 1u },
    { OAF_FOREIGN_TYPE_BOOL, "bool", sizeof(bool), _Alignof(bool) },
    { OAF_FOREIGN_TYPE_I32, "i32", sizeof(int32_t), _Alignof(int32_t) },
    { OAF_FOREIGN_TYPE_I64, "i64", sizeof(int64_t), _Alignof(int64_t) },
    { OAF_FOREIGN_TYPE_F32, "f32", sizeof(float), _Alignof(float) },
    { OAF_FOREIGN_TYPE_F64, "f64", sizeof(double), _Alignof(double) },
    { OAF_FOREIGN_TYPE_POINTER, "pointer", sizeof(void*), _Alignof(void*) }
};

const OafForeignTypeDescriptor* oaf_foreign_type_descriptor(OafForeignTypeKind kind)
{
    size_t index = (size_t)kind;

    if (index >= (sizeof(FOREIGN_TYPE_DESCRIPTORS) / sizeof(FOREIGN_TYPE_DESCRIPTORS[0])))
    {
        return NULL;
    }

    return &FOREIGN_TYPE_DESCRIPTORS[index];
}

int oaf_foreign_type_is_numeric(OafForeignTypeKind kind)
{
    return kind == OAF_FOREIGN_TYPE_I32
        || kind == OAF_FOREIGN_TYPE_I64
        || kind == OAF_FOREIGN_TYPE_F32
        || kind == OAF_FOREIGN_TYPE_F64;
}
