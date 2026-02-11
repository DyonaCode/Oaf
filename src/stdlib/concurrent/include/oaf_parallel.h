#ifndef OAF_STDLIB_PARALLEL_H
#define OAF_STDLIB_PARALLEL_H

#include <stddef.h>
#include <stdint.h>
#include "oaf_thread_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*OafParallelForProc)(size_t index, void* state);
typedef void (*OafParallelMapProc)(size_t index, const void* input, void* output, void* state);
typedef int64_t (*OafParallelReduceProcI64)(size_t index, void* state);

int oaf_parallel_for(
    OafThreadPool* pool,
    size_t count,
    size_t chunk_size,
    OafParallelForProc proc,
    void* state);

int oaf_parallel_map(
    OafThreadPool* pool,
    const void* input,
    void* output,
    size_t count,
    size_t element_size,
    size_t chunk_size,
    OafParallelMapProc proc,
    void* state);

int oaf_parallel_reduce_i64(
    OafThreadPool* pool,
    size_t count,
    size_t chunk_size,
    OafParallelReduceProcI64 proc,
    void* state,
    int64_t* out_result);

#ifdef __cplusplus
}
#endif

#endif
