#include <stddef.h>
#include "null_safety.h"

int oaf_null_is_non_null(const void* pointer)
{
    return pointer != NULL;
}

void* oaf_null_require_non_null(void* pointer)
{
    if (pointer == NULL)
    {
        return NULL;
    }

    return pointer;
}

const void* oaf_null_require_non_null_const(const void* pointer)
{
    if (pointer == NULL)
    {
        return NULL;
    }

    return pointer;
}
