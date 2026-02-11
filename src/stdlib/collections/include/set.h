#ifndef OAFLANG_STDLIB_SET_H
#define OAFLANG_STDLIB_SET_H

#include <stddef.h>
#include "dict.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef OafDictHashProc OafSetHashProc;
typedef OafDictEqualsProc OafSetEqualsProc;

typedef struct OafSet
{
    OafDict storage;
} OafSet;

int oaf_set_init(
    OafSet* set,
    size_t element_size,
    size_t initial_capacity,
    OafSetHashProc hash,
    OafSetEqualsProc equals,
    void* callback_state,
    OafAllocator* allocator);
void oaf_set_destroy(OafSet* set);
void oaf_set_clear(OafSet* set);

size_t oaf_set_count(const OafSet* set);
int oaf_set_reserve(OafSet* set, size_t min_capacity);

int oaf_set_add(OafSet* set, const void* element);
int oaf_set_contains(const OafSet* set, const void* element);
int oaf_set_remove(OafSet* set, const void* element);

#ifdef __cplusplus
}
#endif

#endif
