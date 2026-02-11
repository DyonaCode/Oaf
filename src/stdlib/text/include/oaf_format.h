#ifndef OAFLANG_STDLIB_FORMAT_H
#define OAFLANG_STDLIB_FORMAT_H

#include <stdarg.h>
#include "oaf_string.h"

#ifdef __cplusplus
extern "C" {
#endif

int oaf_format_append(OafString* output, const char* format, ...);
int oaf_format_append_v(OafString* output, const char* format, va_list args);
int oaf_format_assign(OafString* output, const char* format, ...);
int oaf_format_assign_v(OafString* output, const char* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif
