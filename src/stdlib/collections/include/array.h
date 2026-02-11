#ifndef OAF_STDLIB_ARRAY_H
#define OAF_STDLIB_ARRAY_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafArray
{
    void* data;
    size_t length;
    size_t capacity;
    size_t element_size;
    OafAllocator* allocator;
} OafArray;

int oaf_array_init(OafArray* array, size_t element_size, size_t initial_capacity, OafAllocator* allocator);
void oaf_array_destroy(OafArray* array);

int oaf_array_reserve(OafArray* array, size_t min_capacity);
int oaf_array_resize(OafArray* array, size_t new_length);
void oaf_array_clear(OafArray* array);

void* oaf_array_at(OafArray* array, size_t index);
const void* oaf_array_at_const(const OafArray* array, size_t index);
int oaf_array_get(const OafArray* array, size_t index, void* out_element);
int oaf_array_set(OafArray* array, size_t index, const void* element);

int oaf_array_push(OafArray* array, const void* element);
int oaf_array_pop(OafArray* array, void* out_element);
int oaf_array_insert(OafArray* array, size_t index, const void* element);
int oaf_array_remove_at(OafArray* array, size_t index, void* out_element);

#ifdef __cplusplus
}
#endif

#endif
