#include <string.h>
#include "oaf_algorithms.h"

static void swap_elements(unsigned char* left, unsigned char* right, size_t element_size)
{
    size_t index;
    if (left == right)
    {
        return;
    }

    for (index = 0; index < element_size; index++)
    {
        unsigned char temp = left[index];
        left[index] = right[index];
        right[index] = temp;
    }
}

void oaf_alg_sort(void* data, size_t count, size_t element_size, OafAlgorithmCompareProc compare, void* state)
{
    unsigned char* bytes = (unsigned char*)data;
    size_t gap;

    if (bytes == NULL || count < 2 || element_size == 0 || compare == NULL)
    {
        return;
    }

    for (gap = count / 2; gap > 0; gap /= 2)
    {
        size_t i;
        for (i = gap; i < count; i++)
        {
            size_t j = i;
            while (j >= gap)
            {
                unsigned char* left = bytes + ((j - gap) * element_size);
                unsigned char* right = bytes + (j * element_size);
                if (compare(left, right, state) <= 0)
                {
                    break;
                }

                swap_elements(left, right, element_size);
                j -= gap;
            }
        }
    }
}

int oaf_alg_binary_search(
    const void* data,
    size_t count,
    size_t element_size,
    const void* needle,
    OafAlgorithmCompareProc compare,
    void* state,
    size_t* out_index)
{
    const unsigned char* bytes = (const unsigned char*)data;
    size_t low;
    size_t high;

    if (bytes == NULL || needle == NULL || element_size == 0 || compare == NULL || out_index == NULL)
    {
        return 0;
    }

    low = 0;
    high = count;
    while (low < high)
    {
        size_t mid = low + ((high - low) / 2);
        const void* element = bytes + (mid * element_size);
        int cmp = compare(element, needle, state);

        if (cmp == 0)
        {
            *out_index = mid;
            return 1;
        }

        if (cmp < 0)
        {
            low = mid + 1;
        }
        else
        {
            high = mid;
        }
    }

    return 0;
}

void oaf_alg_reverse(void* data, size_t count, size_t element_size)
{
    unsigned char* bytes = (unsigned char*)data;
    size_t i;

    if (bytes == NULL || count < 2 || element_size == 0)
    {
        return;
    }

    for (i = 0; i < count / 2; i++)
    {
        unsigned char* left = bytes + (i * element_size);
        unsigned char* right = bytes + ((count - i - 1) * element_size);
        swap_elements(left, right, element_size);
    }
}

size_t oaf_alg_partition(
    void* data,
    size_t count,
    size_t element_size,
    OafAlgorithmPredicateProc predicate,
    void* state)
{
    unsigned char* bytes = (unsigned char*)data;
    size_t write_index;
    size_t read_index;

    if (bytes == NULL || count == 0 || element_size == 0 || predicate == NULL)
    {
        return 0;
    }

    write_index = 0;
    for (read_index = 0; read_index < count; read_index++)
    {
        unsigned char* element = bytes + (read_index * element_size);
        if (predicate(element, state))
        {
            if (write_index != read_index)
            {
                unsigned char* destination = bytes + (write_index * element_size);
                swap_elements(destination, element, element_size);
            }
            write_index++;
        }
    }

    return write_index;
}
