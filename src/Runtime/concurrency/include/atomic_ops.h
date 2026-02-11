#ifndef OAFLANG_ATOMIC_OPS_H
#define OAFLANG_ATOMIC_OPS_H

#include <stdint.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafAtomicI64
{
    atomic_llong value;
} OafAtomicI64;

typedef struct OafAtomicU64
{
    atomic_ullong value;
} OafAtomicU64;

void oaf_atomic_i64_init(OafAtomicI64* atomic_value, int64_t initial_value);
int64_t oaf_atomic_i64_load(const OafAtomicI64* atomic_value);
void oaf_atomic_i64_store(OafAtomicI64* atomic_value, int64_t value);
int64_t oaf_atomic_i64_fetch_add(OafAtomicI64* atomic_value, int64_t value);
int64_t oaf_atomic_i64_fetch_sub(OafAtomicI64* atomic_value, int64_t value);
int oaf_atomic_i64_compare_exchange(OafAtomicI64* atomic_value, int64_t* expected, int64_t desired);

void oaf_atomic_u64_init(OafAtomicU64* atomic_value, uint64_t initial_value);
uint64_t oaf_atomic_u64_load(const OafAtomicU64* atomic_value);
void oaf_atomic_u64_store(OafAtomicU64* atomic_value, uint64_t value);
uint64_t oaf_atomic_u64_fetch_add(OafAtomicU64* atomic_value, uint64_t value);
uint64_t oaf_atomic_u64_fetch_sub(OafAtomicU64* atomic_value, uint64_t value);
int oaf_atomic_u64_compare_exchange(OafAtomicU64* atomic_value, uint64_t* expected, uint64_t desired);

#ifdef __cplusplus
}
#endif

#endif
