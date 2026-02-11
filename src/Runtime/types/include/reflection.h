#ifndef OAFLANG_REFLECTION_H
#define OAFLANG_REFLECTION_H

#include <stddef.h>
#include "type_info.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t oaf_reflection_field_count(const OafTypeInfo* type);
const OafFieldInfo* oaf_reflection_field_at(const OafTypeInfo* type, size_t index);
const OafFieldInfo* oaf_reflection_find_field(const OafTypeInfo* type, const char* field_name);

size_t oaf_reflection_method_count(const OafTypeInfo* type);
const OafMethodInfo* oaf_reflection_method_at(const OafTypeInfo* type, size_t index);
const OafMethodInfo* oaf_reflection_find_method(const OafTypeInfo* type, const char* method_name);

size_t oaf_reflection_interface_count(const OafTypeInfo* type);
const OafInterfaceInfo* oaf_reflection_interface_at(const OafTypeInfo* type, size_t index);
const OafInterfaceInfo* oaf_reflection_find_interface(const OafTypeInfo* type, const char* interface_name);
int oaf_reflection_implements_interface(const OafTypeInfo* type, const OafInterfaceInfo* interface_info);

#ifdef __cplusplus
}
#endif

#endif
