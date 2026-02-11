#ifndef OAFLANG_MARSHALLING_H
#define OAFLANG_MARSHALLING_H

#include <stdbool.h>
#include <stdint.h>
#include "foreign_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union OafFfiRawValue
{
    bool bool_value;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    void* pointer;
    const void* const_pointer;
} OafFfiRawValue;

typedef struct OafFfiValue
{
    OafForeignTypeKind type;
    OafFfiRawValue as;
} OafFfiValue;

void oaf_ffi_value_clear(OafFfiValue* value);
void oaf_ffi_value_set_bool(OafFfiValue* value, bool data);
void oaf_ffi_value_set_i32(OafFfiValue* value, int32_t data);
void oaf_ffi_value_set_i64(OafFfiValue* value, int64_t data);
void oaf_ffi_value_set_f32(OafFfiValue* value, float data);
void oaf_ffi_value_set_f64(OafFfiValue* value, double data);
void oaf_ffi_value_set_pointer(OafFfiValue* value, void* data);

bool oaf_ffi_value_as_bool(const OafFfiValue* value);
int32_t oaf_ffi_value_as_i32(const OafFfiValue* value);
int64_t oaf_ffi_value_as_i64(const OafFfiValue* value);
float oaf_ffi_value_as_f32(const OafFfiValue* value);
double oaf_ffi_value_as_f64(const OafFfiValue* value);
void* oaf_ffi_value_as_pointer(const OafFfiValue* value);

int oaf_marshal_from_host(const void* source, OafForeignTypeKind type, OafFfiValue* destination);
int oaf_unmarshal_to_host(const OafFfiValue* source, void* destination, OafForeignTypeKind type);

#ifdef __cplusplus
}
#endif

#endif
