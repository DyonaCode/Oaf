#include <stdio.h>
#include <stdint.h>
#include "array.h"
#include "list.h"
#include "dict.h"
#include "set.h"
#include "default_allocator.h"

static int equals_int(const void* element, const void* needle, void* state)
{
    const int* left = (const int*)element;
    const int* right = (const int*)needle;
    (void)state;
    return *left == *right;
}

static int test_array(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafArray array;
    int value;
    int removed;
    int first;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_array_init(&array, sizeof(int), 0, &allocator))
    {
        return 0;
    }

    for (value = 1; value <= 64; value++)
    {
        if (!oaf_array_push(&array, &value))
        {
            ok = 0;
            break;
        }
    }

    ok = ok && array.length == 64 && array.capacity >= 64;
    ok = ok && oaf_array_get(&array, 10, &value) && value == 11;

    value = 111;
    ok = ok && oaf_array_set(&array, 10, &value);
    ok = ok && oaf_array_get(&array, 10, &value) && value == 111;

    value = 999;
    ok = ok && oaf_array_insert(&array, 0, &value);
    ok = ok && oaf_array_get(&array, 0, &first) && first == 999;

    ok = ok && oaf_array_remove_at(&array, 1, &removed) && removed == 1;
    ok = ok && oaf_array_pop(&array, &removed) && removed == 64;
    ok = ok && array.length == 63;

    ok = ok && oaf_array_resize(&array, 80);
    ok = ok && oaf_array_get(&array, 70, &value) && value == 0;
    ok = ok && !oaf_array_get(&array, 120, &value);

    oaf_array_clear(&array);
    ok = ok && array.length == 0;

    oaf_array_destroy(&array);
    return ok && state.active_allocations == 0;
}

static int test_list(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafList list;
    size_t index;
    int value;
    int removed;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_list_init(&list, sizeof(int), 2, &allocator))
    {
        return 0;
    }

    value = 10;
    ok = ok && oaf_list_append(&list, &value);
    value = 30;
    ok = ok && oaf_list_append(&list, &value);
    value = 40;
    ok = ok && oaf_list_append(&list, &value);

    value = 20;
    ok = ok && oaf_list_insert(&list, 1, &value);
    ok = ok && !oaf_list_insert(&list, 99, &value);
    ok = ok && oaf_list_length(&list) == 4;

    ok = ok && oaf_list_remove_at(&list, 2, &removed) && removed == 30;
    ok = ok && oaf_list_length(&list) == 3;

    value = 20;
    ok = ok && oaf_list_find(&list, &value, equals_int, NULL, &index) && index == 1;
    value = 99;
    ok = ok && !oaf_list_find(&list, &value, equals_int, NULL, &index);

    value = 50;
    ok = ok && oaf_list_set(&list, 2, &value);
    ok = ok && oaf_list_pop(&list, &removed) && removed == 50;
    ok = ok && oaf_list_get(&list, 1, &value) && value == 20;

    oaf_list_clear(&list);
    ok = ok && oaf_list_length(&list) == 0;

    oaf_list_destroy(&list);
    return ok && state.active_allocations == 0;
}

static int test_dict(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafDict dict;
    int64_t key;
    int64_t value;
    int64_t removed;
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_dict_init(
            &dict,
            sizeof(int64_t),
            sizeof(int64_t),
            4,
            oaf_dict_hash_i64,
            oaf_dict_equals_i64,
            NULL,
            &allocator))
    {
        return 0;
    }

    for (key = 0; key < 256; key++)
    {
        value = key * 3;
        if (!oaf_dict_set(&dict, &key, &value))
        {
            ok = 0;
            break;
        }
    }

    ok = ok && oaf_dict_count(&dict) == 256;
    ok = ok && oaf_dict_bucket_count(&dict) >= 256;

    key = 42;
    ok = ok && oaf_dict_try_get(&dict, &key, &value) && value == 126;

    value = 4200;
    ok = ok && oaf_dict_set(&dict, &key, &value);
    ok = ok && oaf_dict_try_get(&dict, &key, &value) && value == 4200;

    key = 10;
    ok = ok && oaf_dict_contains_key(&dict, &key);
    key = 999;
    ok = ok && !oaf_dict_contains_key(&dict, &key);

    key = 10;
    ok = ok && oaf_dict_remove(&dict, &key, &removed) && removed == 30;
    ok = ok && oaf_dict_count(&dict) == 255;
    ok = ok && !oaf_dict_remove(&dict, &key, &removed);

    ok = ok && oaf_dict_reserve(&dict, 1024);
    ok = ok && oaf_dict_bucket_count(&dict) >= 1024;

    oaf_dict_clear(&dict);
    ok = ok && oaf_dict_count(&dict) == 0;
    ok = ok && oaf_dict_bucket_count(&dict) == 0;

    oaf_dict_destroy(&dict);
    return ok && state.active_allocations == 0;
}

static int test_set(void)
{
    OafDefaultAllocatorState state;
    OafAllocator allocator;
    OafSet set;
    const char* alpha = "alpha";
    const char* beta = "beta";
    const char* gamma = "gamma";
    int ok = 1;

    oaf_default_allocator_init(&state, &allocator);
    if (!oaf_set_init(
            &set,
            sizeof(const char*),
            4,
            oaf_dict_hash_cstr,
            oaf_dict_equals_cstr,
            NULL,
            &allocator))
    {
        return 0;
    }

    ok = ok && oaf_set_add(&set, &alpha);
    ok = ok && oaf_set_add(&set, &beta);
    ok = ok && oaf_set_add(&set, &alpha);
    ok = ok && oaf_set_count(&set) == 2;

    ok = ok && oaf_set_contains(&set, &beta);
    ok = ok && !oaf_set_contains(&set, &gamma);

    ok = ok && oaf_set_remove(&set, &beta);
    ok = ok && !oaf_set_contains(&set, &beta);
    ok = ok && !oaf_set_remove(&set, &beta);
    ok = ok && oaf_set_count(&set) == 1;

    oaf_set_clear(&set);
    ok = ok && oaf_set_count(&set) == 0;

    oaf_set_destroy(&set);
    return ok && state.active_allocations == 0;
}

int main(void)
{
    if (!test_array() || !test_list() || !test_dict() || !test_set())
    {
        fprintf(stderr, "collections smoke tests failed\n");
        return 1;
    }

    printf("collections smoke tests passed\n");
    return 0;
}
