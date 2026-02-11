#ifndef OAFLANG_LEAK_DETECTOR_H
#define OAFLANG_LEAK_DETECTOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_LEAK_DETECTOR_MAX_RECORDS 2048

typedef struct OafLeakRecord
{
    void* pointer;
    size_t size;
    int active;
} OafLeakRecord;

typedef struct OafLeakDetector
{
    OafLeakRecord records[OAF_LEAK_DETECTOR_MAX_RECORDS];
    size_t active_allocations;
    size_t active_bytes;
    size_t peak_bytes;
    size_t dropped_records;
} OafLeakDetector;

void oaf_leak_detector_init(OafLeakDetector* detector);
int oaf_leak_detector_track_alloc(OafLeakDetector* detector, void* pointer, size_t size);
int oaf_leak_detector_track_free(OafLeakDetector* detector, void* pointer);
size_t oaf_leak_detector_active_allocations(const OafLeakDetector* detector);
size_t oaf_leak_detector_active_bytes(const OafLeakDetector* detector);
size_t oaf_leak_detector_peak_bytes(const OafLeakDetector* detector);
int oaf_leak_detector_has_leaks(const OafLeakDetector* detector);

#ifdef __cplusplus
}
#endif

#endif
