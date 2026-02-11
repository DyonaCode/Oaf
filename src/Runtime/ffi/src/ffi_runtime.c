#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include "ffi_runtime.h"

#ifdef OAFLANG_HAVE_LIBFFI
#include <ffi.h>
#endif

void oaf_ffi_signature_clear(OafFfiSignature* signature)
{
    size_t index;

    if (signature == NULL)
    {
        return;
    }

    signature->return_type = OAF_FOREIGN_TYPE_VOID;
    signature->argument_count = 0;

    for (index = 0; index < OAF_FFI_MAX_ARGS; index++)
    {
        signature->argument_types[index] = OAF_FOREIGN_TYPE_VOID;
    }
}

int oaf_ffi_signature_add_argument(OafFfiSignature* signature, OafForeignTypeKind argument_type)
{
    if (signature == NULL)
    {
        return 0;
    }

    if (signature->argument_count >= OAF_FFI_MAX_ARGS)
    {
        return 0;
    }

    signature->argument_types[signature->argument_count] = argument_type;
    signature->argument_count++;
    return 1;
}

int oaf_ffi_library_open(OafFfiLibrary* library, const char* path)
{
    void* handle;
    int owns_handle;

    if (library == NULL)
    {
        return 0;
    }

    if (path == NULL || path[0] == '\0')
    {
        handle = dlopen(NULL, RTLD_LAZY);
        owns_handle = 0;
    }
    else
    {
        handle = dlopen(path, RTLD_LAZY);
        owns_handle = 1;
    }

    if (handle == NULL)
    {
        library->handle = NULL;
        library->owns_handle = 0;
        return 0;
    }

    library->handle = handle;
    library->owns_handle = owns_handle;
    return 1;
}

void oaf_ffi_library_close(OafFfiLibrary* library)
{
    if (library == NULL || library->handle == NULL)
    {
        return;
    }

    if (library->owns_handle)
    {
        dlclose(library->handle);
    }

    library->handle = NULL;
    library->owns_handle = 0;
}

void* oaf_ffi_library_symbol(const OafFfiLibrary* library, const char* symbol_name)
{
    if (library == NULL || library->handle == NULL || symbol_name == NULL || symbol_name[0] == '\0')
    {
        return NULL;
    }

    return dlsym(library->handle, symbol_name);
}

#ifdef OAFLANG_HAVE_LIBFFI
static ffi_type* ffi_type_for(OafForeignTypeKind type)
{
    switch (type)
    {
        case OAF_FOREIGN_TYPE_VOID:
            return &ffi_type_void;
        case OAF_FOREIGN_TYPE_BOOL:
            return &ffi_type_uint8;
        case OAF_FOREIGN_TYPE_I32:
            return &ffi_type_sint32;
        case OAF_FOREIGN_TYPE_I64:
            return &ffi_type_sint64;
        case OAF_FOREIGN_TYPE_F32:
            return &ffi_type_float;
        case OAF_FOREIGN_TYPE_F64:
            return &ffi_type_double;
        case OAF_FOREIGN_TYPE_POINTER:
            return &ffi_type_pointer;
        default:
            return NULL;
    }
}

static void marshal_ffi_arg(void* storage, OafForeignTypeKind type, const OafFfiValue* value)
{
    switch (type)
    {
        case OAF_FOREIGN_TYPE_BOOL:
            *(uint8_t*)storage = oaf_ffi_value_as_bool(value) ? 1 : 0;
            break;
        case OAF_FOREIGN_TYPE_I32:
            *(int32_t*)storage = oaf_ffi_value_as_i32(value);
            break;
        case OAF_FOREIGN_TYPE_I64:
            *(int64_t*)storage = oaf_ffi_value_as_i64(value);
            break;
        case OAF_FOREIGN_TYPE_F32:
            *(float*)storage = oaf_ffi_value_as_f32(value);
            break;
        case OAF_FOREIGN_TYPE_F64:
            *(double*)storage = oaf_ffi_value_as_f64(value);
            break;
        case OAF_FOREIGN_TYPE_POINTER:
            *(void**)storage = oaf_ffi_value_as_pointer(value);
            break;
        default:
            break;
    }
}
#endif

static OafFfiCallStatus fallback_call(
    const void* function_address,
    const OafFfiSignature* signature,
    const OafFfiValue* arguments,
    OafFfiValue* result)
{
    if (function_address == NULL || signature == NULL)
    {
        return OAF_FFI_CALL_INVALID_ARGUMENT;
    }

    if (signature->argument_count == 0 && signature->return_type == OAF_FOREIGN_TYPE_I64)
    {
        typedef int64_t (*FnI64Void)(void);
        FnI64Void fn = (FnI64Void)function_address;
        if (result != NULL)
        {
            oaf_ffi_value_set_i64(result, fn());
        }
        else
        {
            fn();
        }
        return OAF_FFI_CALL_OK;
    }

    if (signature->argument_count == 1
        && signature->argument_types[0] == OAF_FOREIGN_TYPE_I64
        && signature->return_type == OAF_FOREIGN_TYPE_I64)
    {
        typedef int64_t (*FnI64I64)(int64_t);
        FnI64I64 fn = (FnI64I64)function_address;
        int64_t arg0 = oaf_ffi_value_as_i64(&arguments[0]);
        if (result != NULL)
        {
            oaf_ffi_value_set_i64(result, fn(arg0));
        }
        else
        {
            fn(arg0);
        }
        return OAF_FFI_CALL_OK;
    }

    if (signature->argument_count == 2
        && signature->argument_types[0] == OAF_FOREIGN_TYPE_I64
        && signature->argument_types[1] == OAF_FOREIGN_TYPE_I64
        && signature->return_type == OAF_FOREIGN_TYPE_I64)
    {
        typedef int64_t (*FnI64I64I64)(int64_t, int64_t);
        FnI64I64I64 fn = (FnI64I64I64)function_address;
        int64_t arg0 = oaf_ffi_value_as_i64(&arguments[0]);
        int64_t arg1 = oaf_ffi_value_as_i64(&arguments[1]);
        if (result != NULL)
        {
            oaf_ffi_value_set_i64(result, fn(arg0, arg1));
        }
        else
        {
            fn(arg0, arg1);
        }
        return OAF_FFI_CALL_OK;
    }

    if (signature->argument_count == 2
        && signature->argument_types[0] == OAF_FOREIGN_TYPE_POINTER
        && signature->argument_types[1] == OAF_FOREIGN_TYPE_I64
        && signature->return_type == OAF_FOREIGN_TYPE_I64)
    {
        typedef int64_t (*FnI64PtrI64)(void*, int64_t);
        FnI64PtrI64 fn = (FnI64PtrI64)function_address;
        void* arg0 = oaf_ffi_value_as_pointer(&arguments[0]);
        int64_t arg1 = oaf_ffi_value_as_i64(&arguments[1]);
        if (result != NULL)
        {
            oaf_ffi_value_set_i64(result, fn(arg0, arg1));
        }
        else
        {
            fn(arg0, arg1);
        }
        return OAF_FFI_CALL_OK;
    }

    if (signature->argument_count == 2
        && signature->argument_types[0] == OAF_FOREIGN_TYPE_F64
        && signature->argument_types[1] == OAF_FOREIGN_TYPE_F64
        && signature->return_type == OAF_FOREIGN_TYPE_F64)
    {
        typedef double (*FnF64F64F64)(double, double);
        FnF64F64F64 fn = (FnF64F64F64)function_address;
        double arg0 = oaf_ffi_value_as_f64(&arguments[0]);
        double arg1 = oaf_ffi_value_as_f64(&arguments[1]);
        if (result != NULL)
        {
            oaf_ffi_value_set_f64(result, fn(arg0, arg1));
        }
        else
        {
            fn(arg0, arg1);
        }
        return OAF_FFI_CALL_OK;
    }

    return OAF_FFI_CALL_UNSUPPORTED_SIGNATURE;
}

OafFfiCallStatus oaf_ffi_call_address(
    const void* function_address,
    const OafFfiSignature* signature,
    const OafFfiValue* arguments,
    OafFfiValue* result)
{
    if (function_address == NULL || signature == NULL)
    {
        return OAF_FFI_CALL_INVALID_ARGUMENT;
    }

    if (signature->argument_count > OAF_FFI_MAX_ARGS)
    {
        return OAF_FFI_CALL_INVALID_ARGUMENT;
    }

#ifdef OAFLANG_HAVE_LIBFFI
    {
        ffi_cif cif;
        ffi_type* arg_types[OAF_FFI_MAX_ARGS];
        void* arg_values[OAF_FFI_MAX_ARGS];
        unsigned char arg_storage[OAF_FFI_MAX_ARGS][16];
        unsigned char return_storage[16];
        ffi_type* return_type;
        size_t index;

        return_type = ffi_type_for(signature->return_type);
        if (return_type == NULL)
        {
            return OAF_FFI_CALL_UNSUPPORTED_SIGNATURE;
        }

        memset(return_storage, 0, sizeof(return_storage));

        for (index = 0; index < signature->argument_count; index++)
        {
            ffi_type* ffi_arg_type = ffi_type_for(signature->argument_types[index]);
            if (ffi_arg_type == NULL)
            {
                return OAF_FFI_CALL_UNSUPPORTED_SIGNATURE;
            }

            arg_types[index] = ffi_arg_type;
            arg_values[index] = arg_storage[index];
            memset(arg_storage[index], 0, sizeof(arg_storage[index]));
            marshal_ffi_arg(arg_storage[index], signature->argument_types[index], &arguments[index]);
        }

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, (unsigned int)signature->argument_count, return_type, arg_types) != FFI_OK)
        {
            return OAF_FFI_CALL_UNSUPPORTED_SIGNATURE;
        }

        ffi_call(&cif, FFI_FN(function_address), return_storage, arg_values);

        if (result != NULL)
        {
            switch (signature->return_type)
            {
                case OAF_FOREIGN_TYPE_VOID:
                    oaf_ffi_value_clear(result);
                    break;
                case OAF_FOREIGN_TYPE_BOOL:
                    oaf_ffi_value_set_bool(result, *((uint8_t*)return_storage) != 0);
                    break;
                case OAF_FOREIGN_TYPE_I32:
                    oaf_ffi_value_set_i32(result, *((int32_t*)return_storage));
                    break;
                case OAF_FOREIGN_TYPE_I64:
                    oaf_ffi_value_set_i64(result, *((int64_t*)return_storage));
                    break;
                case OAF_FOREIGN_TYPE_F32:
                    oaf_ffi_value_set_f32(result, *((float*)return_storage));
                    break;
                case OAF_FOREIGN_TYPE_F64:
                    oaf_ffi_value_set_f64(result, *((double*)return_storage));
                    break;
                case OAF_FOREIGN_TYPE_POINTER:
                    oaf_ffi_value_set_pointer(result, *((void**)return_storage));
                    break;
                default:
                    return OAF_FFI_CALL_UNSUPPORTED_SIGNATURE;
            }
        }

        return OAF_FFI_CALL_OK;
    }
#else
    return fallback_call(function_address, signature, arguments, result);
#endif
}

OafFfiCallStatus oaf_ffi_call_symbol(
    const OafFfiLibrary* library,
    const char* symbol_name,
    const OafFfiSignature* signature,
    const OafFfiValue* arguments,
    OafFfiValue* result)
{
    void* symbol;

    if (library == NULL || symbol_name == NULL)
    {
        return OAF_FFI_CALL_INVALID_ARGUMENT;
    }

    symbol = oaf_ffi_library_symbol(library, symbol_name);
    if (symbol == NULL)
    {
        return OAF_FFI_CALL_SYMBOL_NOT_FOUND;
    }

    return oaf_ffi_call_address(symbol, signature, arguments, result);
}
