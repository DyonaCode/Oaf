#ifndef OAFLANG_GC_H
#define OAFLANG_GC_H

#include <stddef.h>
#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OAF_GC_MAX_OBJECTS 512

typedef struct OafGcObject
{
    void* pointer;
    size_t size;
    size_t external_refs;
    int marked;
    int active;
} OafGcObject;

typedef struct OafGarbageCollector
{
    OafAllocator* allocator;
    OafGcObject objects[OAF_GC_MAX_OBJECTS];
    unsigned char edges[OAF_GC_MAX_OBJECTS][OAF_GC_MAX_OBJECTS];
    size_t active_count;
    size_t managed_bytes;
    int enabled;
} OafGarbageCollector;

int oaf_gc_init(OafGarbageCollector* collector, OafAllocator* allocator, int enabled);
void oaf_gc_destroy(OafGarbageCollector* collector);
void oaf_gc_set_enabled(OafGarbageCollector* collector, int enabled);
void* oaf_gc_alloc(OafGarbageCollector* collector, size_t size, size_t alignment);
int oaf_gc_retain(OafGarbageCollector* collector, void* pointer);
int oaf_gc_release(OafGarbageCollector* collector, void* pointer);
int oaf_gc_add_reference(OafGarbageCollector* collector, void* from, void* to);
int oaf_gc_remove_reference(OafGarbageCollector* collector, void* from, void* to);
size_t oaf_gc_collect(OafGarbageCollector* collector);
int oaf_gc_detect_cycles(const OafGarbageCollector* collector);
size_t oaf_gc_object_count(const OafGarbageCollector* collector);
size_t oaf_gc_managed_bytes(const OafGarbageCollector* collector);

#ifdef __cplusplus
}
#endif

#endif
