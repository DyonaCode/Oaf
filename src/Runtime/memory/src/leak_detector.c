#include "leak_detector.h"

static OafLeakRecord* find_record(OafLeakDetector* detector, void* pointer)
{
    size_t index;

    if (detector == NULL || pointer == NULL)
    {
        return NULL;
    }

    for (index = 0; index < OAF_LEAK_DETECTOR_MAX_RECORDS; index++)
    {
        OafLeakRecord* record = &detector->records[index];

        if (record->active && record->pointer == pointer)
        {
            return record;
        }
    }

    return NULL;
}

static OafLeakRecord* find_free_slot(OafLeakDetector* detector)
{
    size_t index;

    if (detector == NULL)
    {
        return NULL;
    }

    for (index = 0; index < OAF_LEAK_DETECTOR_MAX_RECORDS; index++)
    {
        OafLeakRecord* record = &detector->records[index];
        if (!record->active)
        {
            return record;
        }
    }

    return NULL;
}

void oaf_leak_detector_init(OafLeakDetector* detector)
{
    size_t index;

    if (detector == NULL)
    {
        return;
    }

    detector->active_allocations = 0;
    detector->active_bytes = 0;
    detector->peak_bytes = 0;
    detector->dropped_records = 0;

    for (index = 0; index < OAF_LEAK_DETECTOR_MAX_RECORDS; index++)
    {
        detector->records[index].pointer = NULL;
        detector->records[index].size = 0;
        detector->records[index].active = 0;
    }
}

int oaf_leak_detector_track_alloc(OafLeakDetector* detector, void* pointer, size_t size)
{
    OafLeakRecord* record;

    if (detector == NULL || pointer == NULL)
    {
        return 0;
    }

    record = find_record(detector, pointer);
    if (record != NULL)
    {
        detector->active_bytes -= record->size;
        record->size = size;
        detector->active_bytes += size;
        if (detector->active_bytes > detector->peak_bytes)
        {
            detector->peak_bytes = detector->active_bytes;
        }
        return 1;
    }

    record = find_free_slot(detector);
    if (record == NULL)
    {
        detector->dropped_records++;
        return 0;
    }

    record->pointer = pointer;
    record->size = size;
    record->active = 1;
    detector->active_allocations++;
    detector->active_bytes += size;

    if (detector->active_bytes > detector->peak_bytes)
    {
        detector->peak_bytes = detector->active_bytes;
    }

    return 1;
}

int oaf_leak_detector_track_free(OafLeakDetector* detector, void* pointer)
{
    OafLeakRecord* record;

    if (detector == NULL || pointer == NULL)
    {
        return 0;
    }

    record = find_record(detector, pointer);
    if (record == NULL)
    {
        return 0;
    }

    if (detector->active_allocations > 0)
    {
        detector->active_allocations--;
    }

    if (detector->active_bytes >= record->size)
    {
        detector->active_bytes -= record->size;
    }
    else
    {
        detector->active_bytes = 0;
    }

    record->active = 0;
    record->pointer = NULL;
    record->size = 0;
    return 1;
}

size_t oaf_leak_detector_active_allocations(const OafLeakDetector* detector)
{
    if (detector == NULL)
    {
        return 0;
    }

    return detector->active_allocations;
}

size_t oaf_leak_detector_active_bytes(const OafLeakDetector* detector)
{
    if (detector == NULL)
    {
        return 0;
    }

    return detector->active_bytes;
}

size_t oaf_leak_detector_peak_bytes(const OafLeakDetector* detector)
{
    if (detector == NULL)
    {
        return 0;
    }

    return detector->peak_bytes;
}

int oaf_leak_detector_has_leaks(const OafLeakDetector* detector)
{
    if (detector == NULL)
    {
        return 0;
    }

    return detector->active_allocations > 0;
}
