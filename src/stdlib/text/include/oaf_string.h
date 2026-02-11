#ifndef OAFLANG_STDLIB_STRING_H
#define OAFLANG_STDLIB_STRING_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafString
{
    char* data;
    size_t length;
    size_t capacity;
    OafAllocator* allocator;
} OafString;

int oaf_string_init(OafString* string, OafAllocator* allocator);
int oaf_string_init_from_cstr(OafString* string, const char* text, OafAllocator* allocator);
void oaf_string_destroy(OafString* string);

int oaf_string_reserve(OafString* string, size_t min_capacity);
void oaf_string_clear(OafString* string);

int oaf_string_append_n(OafString* string, const char* text, size_t length);
int oaf_string_append_cstr(OafString* string, const char* text);
int oaf_string_append_char(OafString* string, char value);

int oaf_string_equals_cstr(const OafString* string, const char* text);
int oaf_string_starts_with(const OafString* string, const char* prefix);
int oaf_string_ends_with(const OafString* string, const char* suffix);
void oaf_string_to_upper_ascii(OafString* string);
void oaf_string_to_lower_ascii(OafString* string);
void oaf_string_trim_ascii(OafString* string);

#ifdef __cplusplus
}
#endif

#endif
