#ifndef OAF_STDLIB_ALGORITHMS_H
#define OAF_STDLIB_ALGORITHMS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*OafAlgorithmCompareProc)(const void* left, const void* right, void* state);
typedef int (*OafAlgorithmPredicateProc)(const void* element, void* state);

void oaf_alg_sort(void* data, size_t count, size_t element_size, OafAlgorithmCompareProc compare, void* state);
int oaf_alg_binary_search(
    const void* data,
    size_t count,
    size_t element_size,
    const void* needle,
    OafAlgorithmCompareProc compare,
    void* state,
    size_t* out_index);
void oaf_alg_reverse(void* data, size_t count, size_t element_size);
size_t oaf_alg_partition(
    void* data,
    size_t count,
    size_t element_size,
    OafAlgorithmPredicateProc predicate,
    void* state);

#ifdef __cplusplus
}
#endif

#endif
