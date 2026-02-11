#ifndef OAF_STDLIB_LIST_H
#define OAF_STDLIB_LIST_H

#include <stddef.h>
#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*OafListEqualsProc)(const void* element, const void* needle, void* state);

typedef struct OafList
{
    OafArray storage;
} OafList;

int oaf_list_init(OafList* list, size_t element_size, size_t initial_capacity, OafAllocator* allocator);
void oaf_list_destroy(OafList* list);

size_t oaf_list_length(const OafList* list);
size_t oaf_list_capacity(const OafList* list);
void oaf_list_clear(OafList* list);

void* oaf_list_at(OafList* list, size_t index);
const void* oaf_list_at_const(const OafList* list, size_t index);
int oaf_list_get(const OafList* list, size_t index, void* out_element);
int oaf_list_set(OafList* list, size_t index, const void* element);

int oaf_list_append(OafList* list, const void* element);
int oaf_list_insert(OafList* list, size_t index, const void* element);
int oaf_list_remove_at(OafList* list, size_t index, void* out_element);
int oaf_list_pop(OafList* list, void* out_element);

int oaf_list_find(const OafList* list, const void* needle, OafListEqualsProc equals, void* state, size_t* out_index);

#ifdef __cplusplus
}
#endif

#endif
