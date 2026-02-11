#ifndef OAFLANG_BOUNDS_H
#define OAFLANG_BOUNDS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int oaf_bounds_check_index(size_t index, size_t length);
int oaf_bounds_check_range(size_t start, size_t count, size_t length);
void* oaf_bounds_checked_offset(void* base, size_t element_size, size_t index, size_t length);
const void* oaf_bounds_checked_offset_const(const void* base, size_t element_size, size_t index, size_t length);

#ifdef __cplusplus
}
#endif

#endif
