#include <stdint.h>
#include "bounds.h"

int oaf_bounds_check_index(size_t index, size_t length)
{
    return index < length;
}

int oaf_bounds_check_range(size_t start, size_t count, size_t length)
{
    if (start > length)
    {
        return 0;
    }

    if (count > (length - start))
    {
        return 0;
    }

    return 1;
}

void* oaf_bounds_checked_offset(void* base, size_t element_size, size_t index, size_t length)
{
    uintptr_t address;

    if (base == NULL || !oaf_bounds_check_index(index, length))
    {
        return NULL;
    }

    address = (uintptr_t)base + (index * element_size);
    return (void*)address;
}

const void* oaf_bounds_checked_offset_const(const void* base, size_t element_size, size_t index, size_t length)
{
    uintptr_t address;

    if (base == NULL || !oaf_bounds_check_index(index, length))
    {
        return NULL;
    }

    address = (uintptr_t)base + (index * element_size);
    return (const void*)address;
}
