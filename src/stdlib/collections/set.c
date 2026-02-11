#include <stdint.h>
#include "set.h"

int oaf_set_init(
    OafSet* set,
    size_t element_size,
    size_t initial_capacity,
    OafSetHashProc hash,
    OafSetEqualsProc equals,
    void* callback_state,
    OafAllocator* allocator)
{
    if (set == NULL)
    {
        return 0;
    }

    return oaf_dict_init(
        &set->storage,
        element_size,
        sizeof(uint8_t),
        initial_capacity,
        hash,
        equals,
        callback_state,
        allocator);
}

void oaf_set_destroy(OafSet* set)
{
    if (set == NULL)
    {
        return;
    }

    oaf_dict_destroy(&set->storage);
}

void oaf_set_clear(OafSet* set)
{
    if (set == NULL)
    {
        return;
    }

    oaf_dict_clear(&set->storage);
}

size_t oaf_set_count(const OafSet* set)
{
    if (set == NULL)
    {
        return 0;
    }

    return oaf_dict_count(&set->storage);
}

int oaf_set_reserve(OafSet* set, size_t min_capacity)
{
    if (set == NULL)
    {
        return 0;
    }

    return oaf_dict_reserve(&set->storage, min_capacity);
}

int oaf_set_add(OafSet* set, const void* element)
{
    const uint8_t marker = 1u;

    if (set == NULL || element == NULL)
    {
        return 0;
    }

    return oaf_dict_set(&set->storage, element, &marker);
}

int oaf_set_contains(const OafSet* set, const void* element)
{
    if (set == NULL || element == NULL)
    {
        return 0;
    }

    return oaf_dict_contains_key(&set->storage, element);
}

int oaf_set_remove(OafSet* set, const void* element)
{
    uint8_t removed_marker = 0u;

    if (set == NULL || element == NULL)
    {
        return 0;
    }

    return oaf_dict_remove(&set->storage, element, &removed_marker);
}
