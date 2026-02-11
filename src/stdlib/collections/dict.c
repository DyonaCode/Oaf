#include <stdint.h>
#include <string.h>
#include "dict.h"

struct OafDictNode
{
    struct OafDictNode* next;
    size_t hash;
    unsigned char payload[];
};

static size_t fnv1a_bytes(const unsigned char* bytes, size_t length)
{
    size_t index;
#if SIZE_MAX == UINT64_MAX
    size_t hash = 1469598103934665603ull;
    const size_t prime = 1099511628211ull;
#else
    size_t hash = 2166136261u;
    const size_t prime = 16777619u;
#endif

    for (index = 0; index < length; index++)
    {
        hash ^= (size_t)bytes[index];
        hash *= prime;
    }

    return hash;
}

static size_t bucket_index_for_hash(size_t hash, size_t bucket_count)
{
    if (bucket_count == 0)
    {
        return 0;
    }

    return hash % bucket_count;
}

static unsigned char* node_key(const OafDict* dict, OafDictNode* node)
{
    (void)dict;
    return node->payload;
}

static unsigned char* node_value(const OafDict* dict, OafDictNode* node)
{
    return node->payload + dict->key_size;
}

static const unsigned char* node_key_const(const OafDict* dict, const OafDictNode* node)
{
    (void)dict;
    return node->payload;
}

static const unsigned char* node_value_const(const OafDict* dict, const OafDictNode* node)
{
    return node->payload + dict->key_size;
}

static int keys_equal(const OafDict* dict, const void* left, const void* right)
{
    if (dict->equals != NULL)
    {
        return dict->equals(left, right, dict->callback_state);
    }

    return memcmp(left, right, dict->key_size) == 0;
}

static size_t hash_key(const OafDict* dict, const void* key)
{
    if (dict->hash != NULL)
    {
        return dict->hash(key, dict->callback_state);
    }

    return oaf_dict_hash_bytes(key, dict->key_size);
}

static int node_payload_size(const OafDict* dict, size_t* out_size)
{
    if (dict == NULL || out_size == NULL)
    {
        return 0;
    }

    if (dict->key_size > (SIZE_MAX - dict->value_size))
    {
        return 0;
    }

    if ((dict->key_size + dict->value_size) > (SIZE_MAX - sizeof(OafDictNode)))
    {
        return 0;
    }

    *out_size = sizeof(OafDictNode) + dict->key_size + dict->value_size;
    return 1;
}

static void free_chain(OafDict* dict, OafDictNode* head)
{
    OafDictNode* node = head;

    while (node != NULL)
    {
        OafDictNode* next = node->next;
        oaf_allocator_free(dict->allocator, node);
        node = next;
    }
}

static int ensure_bucket_array(OafDict* dict, size_t bucket_count)
{
    size_t bytes;

    if (dict == NULL || dict->allocator == NULL)
    {
        return 0;
    }

    if (bucket_count == 0)
    {
        if (dict->buckets != NULL)
        {
            oaf_allocator_free(dict->allocator, dict->buckets);
        }
        dict->buckets = NULL;
        dict->bucket_count = 0;
        return 1;
    }

    if (bucket_count > (SIZE_MAX / sizeof(OafDictNode*)))
    {
        return 0;
    }

    bytes = bucket_count * sizeof(OafDictNode*);
    dict->buckets = (OafDictNode**)oaf_allocator_alloc(dict->allocator, bytes, _Alignof(OafDictNode*));
    if (dict->buckets == NULL)
    {
        return 0;
    }

    memset(dict->buckets, 0, bytes);
    dict->bucket_count = bucket_count;
    return 1;
}

static size_t next_bucket_count(size_t min_capacity)
{
    size_t capacity = 8u;

    while (capacity < min_capacity)
    {
        if (capacity > (SIZE_MAX / 2u))
        {
            return min_capacity;
        }

        capacity *= 2u;
    }

    return capacity;
}

int oaf_dict_init(
    OafDict* dict,
    size_t key_size,
    size_t value_size,
    size_t initial_capacity,
    OafDictHashProc hash,
    OafDictEqualsProc equals,
    void* callback_state,
    OafAllocator* allocator)
{
    if (dict == NULL || allocator == NULL || key_size == 0)
    {
        return 0;
    }

    dict->allocator = allocator;
    dict->key_size = key_size;
    dict->value_size = value_size;
    dict->count = 0;
    dict->bucket_count = 0;
    dict->hash = hash;
    dict->equals = equals;
    dict->callback_state = callback_state;
    dict->buckets = NULL;

    if (initial_capacity == 0)
    {
        return 1;
    }

    return oaf_dict_reserve(dict, initial_capacity);
}

void oaf_dict_destroy(OafDict* dict)
{
    if (dict == NULL)
    {
        return;
    }

    oaf_dict_clear(dict);

    dict->allocator = NULL;
    dict->key_size = 0;
    dict->value_size = 0;
    dict->hash = NULL;
    dict->equals = NULL;
    dict->callback_state = NULL;
}

void oaf_dict_clear(OafDict* dict)
{
    size_t bucket_index;

    if (dict == NULL || dict->allocator == NULL)
    {
        return;
    }

    for (bucket_index = 0; bucket_index < dict->bucket_count; bucket_index++)
    {
        free_chain(dict, dict->buckets[bucket_index]);
        dict->buckets[bucket_index] = NULL;
    }

    if (dict->buckets != NULL)
    {
        oaf_allocator_free(dict->allocator, dict->buckets);
        dict->buckets = NULL;
    }

    dict->count = 0;
    dict->bucket_count = 0;
}

size_t oaf_dict_count(const OafDict* dict)
{
    if (dict == NULL)
    {
        return 0;
    }

    return dict->count;
}

size_t oaf_dict_bucket_count(const OafDict* dict)
{
    if (dict == NULL)
    {
        return 0;
    }

    return dict->bucket_count;
}

int oaf_dict_reserve(OafDict* dict, size_t min_capacity)
{
    OafDictNode** previous_buckets;
    size_t previous_bucket_count;
    size_t target_bucket_count;
    size_t bucket_index;

    if (dict == NULL)
    {
        return 0;
    }

    if (dict->bucket_count >= min_capacity)
    {
        return 1;
    }

    target_bucket_count = next_bucket_count(min_capacity);
    if (target_bucket_count < 8u)
    {
        target_bucket_count = 8u;
    }

    previous_buckets = dict->buckets;
    previous_bucket_count = dict->bucket_count;

    dict->buckets = NULL;
    dict->bucket_count = 0;
    if (!ensure_bucket_array(dict, target_bucket_count))
    {
        dict->buckets = previous_buckets;
        dict->bucket_count = previous_bucket_count;
        return 0;
    }

    for (bucket_index = 0; bucket_index < previous_bucket_count; bucket_index++)
    {
        OafDictNode* node = previous_buckets[bucket_index];
        while (node != NULL)
        {
            OafDictNode* next = node->next;
            size_t target_index = bucket_index_for_hash(node->hash, dict->bucket_count);
            node->next = dict->buckets[target_index];
            dict->buckets[target_index] = node;
            node = next;
        }
    }

    if (previous_buckets != NULL)
    {
        oaf_allocator_free(dict->allocator, previous_buckets);
    }

    return 1;
}

int oaf_dict_set(OafDict* dict, const void* key, const void* value)
{
    size_t key_hash;
    size_t bucket_index;
    OafDictNode* node;
    size_t allocation_size;
    size_t target_buckets;

    if (dict == NULL || key == NULL || value == NULL)
    {
        return 0;
    }

    target_buckets = dict->bucket_count == 0 ? 8u : dict->bucket_count;
    if (dict->count >= target_buckets)
    {
        if (!oaf_dict_reserve(dict, target_buckets * 2u))
        {
            return 0;
        }
    }
    else if (dict->bucket_count == 0)
    {
        if (!oaf_dict_reserve(dict, 8u))
        {
            return 0;
        }
    }

    key_hash = hash_key(dict, key);
    bucket_index = bucket_index_for_hash(key_hash, dict->bucket_count);
    node = dict->buckets[bucket_index];

    while (node != NULL)
    {
        if (node->hash == key_hash && keys_equal(dict, node_key_const(dict, node), key))
        {
            memcpy(node_value(dict, node), value, dict->value_size);
            return 1;
        }
        node = node->next;
    }

    if (!node_payload_size(dict, &allocation_size))
    {
        return 0;
    }

    node = (OafDictNode*)oaf_allocator_alloc(dict->allocator, allocation_size, _Alignof(OafDictNode));
    if (node == NULL)
    {
        return 0;
    }

    node->hash = key_hash;
    memcpy(node_key(dict, node), key, dict->key_size);
    memcpy(node_value(dict, node), value, dict->value_size);

    node->next = dict->buckets[bucket_index];
    dict->buckets[bucket_index] = node;
    dict->count++;
    return 1;
}

int oaf_dict_try_get(const OafDict* dict, const void* key, void* out_value)
{
    size_t key_hash;
    size_t bucket_index;
    OafDictNode* node;

    if (dict == NULL || key == NULL || out_value == NULL || dict->bucket_count == 0)
    {
        return 0;
    }

    key_hash = hash_key(dict, key);
    bucket_index = bucket_index_for_hash(key_hash, dict->bucket_count);
    node = dict->buckets[bucket_index];

    while (node != NULL)
    {
        if (node->hash == key_hash && keys_equal(dict, node_key_const(dict, node), key))
        {
            memcpy(out_value, node_value_const(dict, node), dict->value_size);
            return 1;
        }

        node = node->next;
    }

    return 0;
}

int oaf_dict_contains_key(const OafDict* dict, const void* key)
{
    size_t key_hash;
    size_t bucket_index;
    OafDictNode* node;

    if (dict == NULL || key == NULL || dict->bucket_count == 0)
    {
        return 0;
    }

    key_hash = hash_key(dict, key);
    bucket_index = bucket_index_for_hash(key_hash, dict->bucket_count);
    node = dict->buckets[bucket_index];

    while (node != NULL)
    {
        if (node->hash == key_hash && keys_equal(dict, node_key_const(dict, node), key))
        {
            return 1;
        }

        node = node->next;
    }

    return 0;
}

int oaf_dict_remove(OafDict* dict, const void* key, void* out_value)
{
    size_t key_hash;
    size_t bucket_index;
    OafDictNode* previous = NULL;
    OafDictNode* node;

    if (dict == NULL || key == NULL || dict->bucket_count == 0)
    {
        return 0;
    }

    key_hash = hash_key(dict, key);
    bucket_index = bucket_index_for_hash(key_hash, dict->bucket_count);
    node = dict->buckets[bucket_index];

    while (node != NULL)
    {
        if (node->hash == key_hash && keys_equal(dict, node_key_const(dict, node), key))
        {
            if (out_value != NULL)
            {
                memcpy(out_value, node_value_const(dict, node), dict->value_size);
            }

            if (previous == NULL)
            {
                dict->buckets[bucket_index] = node->next;
            }
            else
            {
                previous->next = node->next;
            }

            oaf_allocator_free(dict->allocator, node);
            dict->count--;
            return 1;
        }

        previous = node;
        node = node->next;
    }

    return 0;
}

size_t oaf_dict_hash_bytes(const void* data, size_t length)
{
    if (data == NULL || length == 0)
    {
        return 0;
    }

    return fnv1a_bytes((const unsigned char*)data, length);
}

size_t oaf_dict_hash_i64(const void* key, void* state)
{
    const int64_t* value = (const int64_t*)key;
    (void)state;

    if (value == NULL)
    {
        return 0;
    }

    return oaf_dict_hash_bytes(value, sizeof(int64_t));
}

size_t oaf_dict_hash_cstr(const void* key, void* state)
{
    const char* const* text = (const char* const*)key;
    (void)state;

    if (text == NULL || *text == NULL)
    {
        return 0;
    }

    return oaf_dict_hash_bytes(*text, strlen(*text));
}

int oaf_dict_equals_i64(const void* left, const void* right, void* state)
{
    const int64_t* left_value = (const int64_t*)left;
    const int64_t* right_value = (const int64_t*)right;
    (void)state;

    if (left_value == NULL || right_value == NULL)
    {
        return 0;
    }

    return *left_value == *right_value;
}

int oaf_dict_equals_cstr(const void* left, const void* right, void* state)
{
    const char* const* left_text = (const char* const*)left;
    const char* const* right_text = (const char* const*)right;
    (void)state;

    if (left_text == NULL || right_text == NULL || *left_text == NULL || *right_text == NULL)
    {
        return 0;
    }

    return strcmp(*left_text, *right_text) == 0;
}
