#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ffi_support.h"

static int64_t native_add_i64(int64_t left, int64_t right)
{
    return left + right;
}

static double native_mul_f64(double left, double right)
{
    return left * right;
}

static int64_t native_apply_callback(void* callback_pointer, int64_t value)
{
    OafFfiTrampolineI64I64 callback = (OafFfiTrampolineI64I64)callback_pointer;
    if (callback == NULL)
    {
        return 0;
    }

    return callback(value);
}

static int64_t callback_increment(const OafFfiValue* args, size_t arg_count, void* user_data)
{
    int64_t increment = *(int64_t*)user_data;
    if (arg_count != 1)
    {
        return 0;
    }

    return oaf_ffi_value_as_i64(&args[0]) + increment;
}

static int test_foreign_types_and_marshalling(void)
{
    const OafForeignTypeDescriptor* type = oaf_foreign_type_descriptor(OAF_FOREIGN_TYPE_I64);
    OafFfiValue value;
    int64_t roundtrip;

    if (type == NULL || strcmp(type->name, "i64") != 0 || !oaf_foreign_type_is_numeric(type->kind))
    {
        return 0;
    }

    oaf_ffi_value_set_i64(&value, 99);
    if (oaf_ffi_value_as_i32(&value) != 99)
    {
        return 0;
    }

    if (!oaf_unmarshal_to_host(&value, &roundtrip, OAF_FOREIGN_TYPE_I64) || roundtrip != 99)
    {
        return 0;
    }

    roundtrip = 1234;
    if (!oaf_marshal_from_host(&roundtrip, OAF_FOREIGN_TYPE_I64, &value))
    {
        return 0;
    }

    return oaf_ffi_value_as_i64(&value) == 1234;
}

static int test_ffi_calls(void)
{
    OafFfiSignature signature;
    OafFfiValue args[2];
    OafFfiValue result;
    OafFfiCallStatus status;

    oaf_ffi_signature_clear(&signature);
    signature.return_type = OAF_FOREIGN_TYPE_I64;
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_I64);
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_I64);
    oaf_ffi_value_set_i64(&args[0], 40);
    oaf_ffi_value_set_i64(&args[1], 2);
    status = oaf_ffi_call_address((const void*)native_add_i64, &signature, args, &result);
    if (status != OAF_FFI_CALL_OK || oaf_ffi_value_as_i64(&result) != 42)
    {
        return 0;
    }

    oaf_ffi_signature_clear(&signature);
    signature.return_type = OAF_FOREIGN_TYPE_F64;
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_F64);
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_F64);
    oaf_ffi_value_set_f64(&args[0], 6.0);
    oaf_ffi_value_set_f64(&args[1], 7.0);
    status = oaf_ffi_call_address((const void*)native_mul_f64, &signature, args, &result);
    if (status != OAF_FFI_CALL_OK || (int)oaf_ffi_value_as_f64(&result) != 42)
    {
        return 0;
    }

    return 1;
}

static int test_callback_registration_and_trampoline(void)
{
    OafFfiCallbackRegistry registry;
    OafFfiTrampolineI64I64 trampoline;
    OafFfiSignature signature;
    OafFfiValue args[2];
    OafFfiValue result;
    OafFfiValue callback_args[1];
    int64_t increment = 1;
    uint64_t callback_id;
    int invoke_success = 0;

    oaf_ffi_callback_registry_init(&registry);
    callback_id = oaf_ffi_callback_register(&registry, callback_increment, &increment);
    if (callback_id == 0)
    {
        return 0;
    }

    if (!oaf_ffi_callback_acquire_trampoline_i64_i64(&registry, callback_id, &trampoline))
    {
        oaf_ffi_callback_unregister(&registry, callback_id);
        return 0;
    }

    oaf_ffi_value_set_i64(&callback_args[0], 41);
    if (oaf_ffi_callback_invoke_i64(&registry, callback_id, callback_args, 1, &invoke_success) != 42 || !invoke_success)
    {
        oaf_ffi_callback_release_trampoline(trampoline);
        oaf_ffi_callback_unregister(&registry, callback_id);
        return 0;
    }

    oaf_ffi_signature_clear(&signature);
    signature.return_type = OAF_FOREIGN_TYPE_I64;
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_POINTER);
    oaf_ffi_signature_add_argument(&signature, OAF_FOREIGN_TYPE_I64);
    oaf_ffi_value_set_pointer(&args[0], (void*)trampoline);
    oaf_ffi_value_set_i64(&args[1], 41);

    if (oaf_ffi_call_address((const void*)native_apply_callback, &signature, args, &result) != OAF_FFI_CALL_OK
        || oaf_ffi_value_as_i64(&result) != 42)
    {
        oaf_ffi_callback_release_trampoline(trampoline);
        oaf_ffi_callback_unregister(&registry, callback_id);
        return 0;
    }

    oaf_ffi_callback_release_trampoline(trampoline);
    return oaf_ffi_callback_unregister(&registry, callback_id);
}

int main(void)
{
    int ok = 1;
    ok = ok && test_foreign_types_and_marshalling();
    ok = ok && test_ffi_calls();
    ok = ok && test_callback_registration_and_trampoline();

    if (!ok)
    {
        fprintf(stderr, "ffi smoke tests failed\n");
        return 1;
    }

    printf("ffi smoke tests passed\n");
    return 0;
}
