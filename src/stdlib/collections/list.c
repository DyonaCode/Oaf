#include <string.h>
#include "list.h"

int oaf_list_init(OafList* list, size_t element_size, size_t initial_capacity, OafAllocator* allocator)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_init(&list->storage, element_size, initial_capacity, allocator);
}

void oaf_list_destroy(OafList* list)
{
    if (list == NULL)
    {
        return;
    }

    oaf_array_destroy(&list->storage);
}

size_t oaf_list_length(const OafList* list)
{
    if (list == NULL)
    {
        return 0;
    }

    return list->storage.length;
}

size_t oaf_list_capacity(const OafList* list)
{
    if (list == NULL)
    {
        return 0;
    }

    return list->storage.capacity;
}

void oaf_list_clear(OafList* list)
{
    if (list == NULL)
    {
        return;
    }

    oaf_array_clear(&list->storage);
}

void* oaf_list_at(OafList* list, size_t index)
{
    if (list == NULL)
    {
        return NULL;
    }

    return oaf_array_at(&list->storage, index);
}

const void* oaf_list_at_const(const OafList* list, size_t index)
{
    if (list == NULL)
    {
        return NULL;
    }

    return oaf_array_at_const(&list->storage, index);
}

int oaf_list_get(const OafList* list, size_t index, void* out_element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_get(&list->storage, index, out_element);
}

int oaf_list_set(OafList* list, size_t index, const void* element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_set(&list->storage, index, element);
}

int oaf_list_append(OafList* list, const void* element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_push(&list->storage, element);
}

int oaf_list_insert(OafList* list, size_t index, const void* element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_insert(&list->storage, index, element);
}

int oaf_list_remove_at(OafList* list, size_t index, void* out_element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_remove_at(&list->storage, index, out_element);
}

int oaf_list_pop(OafList* list, void* out_element)
{
    if (list == NULL)
    {
        return 0;
    }

    return oaf_array_pop(&list->storage, out_element);
}

int oaf_list_find(const OafList* list, const void* needle, OafListEqualsProc equals, void* state, size_t* out_index)
{
    size_t index;

    if (list == NULL || out_index == NULL)
    {
        return 0;
    }

    for (index = 0; index < list->storage.length; index++)
    {
        const void* element = oaf_array_at_const(&list->storage, index);
        int matched;

        if (equals != NULL)
        {
            matched = equals(element, needle, state);
        }
        else if (needle != NULL)
        {
            matched = memcmp(element, needle, list->storage.element_size) == 0;
        }
        else
        {
            matched = 0;
        }

        if (matched)
        {
            *out_index = index;
            return 1;
        }
    }

    return 0;
}
