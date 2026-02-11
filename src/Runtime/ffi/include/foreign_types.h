#ifndef OAF_FOREIGN_TYPES_H
#define OAF_FOREIGN_TYPES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OafForeignTypeKind
{
    OAF_FOREIGN_TYPE_VOID = 0,
    OAF_FOREIGN_TYPE_BOOL = 1,
    OAF_FOREIGN_TYPE_I32 = 2,
    OAF_FOREIGN_TYPE_I64 = 3,
    OAF_FOREIGN_TYPE_F32 = 4,
    OAF_FOREIGN_TYPE_F64 = 5,
    OAF_FOREIGN_TYPE_POINTER = 6
} OafForeignTypeKind;

typedef struct OafForeignTypeDescriptor
{
    OafForeignTypeKind kind;
    const char* name;
    size_t size;
    size_t alignment;
} OafForeignTypeDescriptor;

const OafForeignTypeDescriptor* oaf_foreign_type_descriptor(OafForeignTypeKind kind);
int oaf_foreign_type_is_numeric(OafForeignTypeKind kind);

#ifdef __cplusplus
}
#endif

#endif
