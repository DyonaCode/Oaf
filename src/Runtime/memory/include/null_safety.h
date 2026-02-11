#ifndef OAFLANG_NULL_SAFETY_H
#define OAFLANG_NULL_SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

int oaf_null_is_non_null(const void* pointer);
void* oaf_null_require_non_null(void* pointer);
const void* oaf_null_require_non_null_const(const void* pointer);

#ifdef __cplusplus
}
#endif

#endif
