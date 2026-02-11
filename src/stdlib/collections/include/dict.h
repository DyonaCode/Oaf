#ifndef OAFLANG_STDLIB_DICT_H
#define OAFLANG_STDLIB_DICT_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*OafDictHashProc)(const void* key, void* state);
typedef int (*OafDictEqualsProc)(const void* left, const void* right, void* state);

typedef struct OafDictNode OafDictNode;

typedef struct OafDict
{
    OafAllocator* allocator;
    size_t key_size;
    size_t value_size;
    size_t count;
    size_t bucket_count;
    OafDictHashProc hash;
    OafDictEqualsProc equals;
    void* callback_state;
    OafDictNode** buckets;
} OafDict;

int oaf_dict_init(
    OafDict* dict,
    size_t key_size,
    size_t value_size,
    size_t initial_capacity,
    OafDictHashProc hash,
    OafDictEqualsProc equals,
    void* callback_state,
    OafAllocator* allocator);
void oaf_dict_destroy(OafDict* dict);
void oaf_dict_clear(OafDict* dict);

size_t oaf_dict_count(const OafDict* dict);
size_t oaf_dict_bucket_count(const OafDict* dict);
int oaf_dict_reserve(OafDict* dict, size_t min_capacity);

int oaf_dict_set(OafDict* dict, const void* key, const void* value);
int oaf_dict_try_get(const OafDict* dict, const void* key, void* out_value);
int oaf_dict_contains_key(const OafDict* dict, const void* key);
int oaf_dict_remove(OafDict* dict, const void* key, void* out_value);

size_t oaf_dict_hash_bytes(const void* data, size_t length);
size_t oaf_dict_hash_i64(const void* key, void* state);
size_t oaf_dict_hash_cstr(const void* key, void* state);
int oaf_dict_equals_i64(const void* left, const void* right, void* state);
int oaf_dict_equals_cstr(const void* left, const void* right, void* state);

#ifdef __cplusplus
}
#endif

#endif
