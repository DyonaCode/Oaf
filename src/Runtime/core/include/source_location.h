#ifndef OAF_SOURCE_LOCATION_H
#define OAF_SOURCE_LOCATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OafSourceLocation
{
    const char* file_name;
    uint32_t line;
    uint32_t column;
} OafSourceLocation;

#ifdef __cplusplus
}
#endif

#endif
