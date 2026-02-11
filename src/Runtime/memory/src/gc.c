#include "gc.h"

static int find_object_index(const OafGarbageCollector* collector, const void* pointer)
{
    size_t index;

    if (collector == NULL || pointer == NULL)
    {
        return -1;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        const OafGcObject* object = &collector->objects[index];
        if (object->active && object->pointer == pointer)
        {
            return (int)index;
        }
    }

    return -1;
}

static int find_free_index(const OafGarbageCollector* collector)
{
    size_t index;

    if (collector == NULL)
    {
        return -1;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        if (!collector->objects[index].active)
        {
            return (int)index;
        }
    }

    return -1;
}

static void clear_edges_for_index(OafGarbageCollector* collector, int index)
{
    size_t column;
    size_t row;

    for (column = 0; column < OAF_GC_MAX_OBJECTS; column++)
    {
        collector->edges[(size_t)index][column] = 0;
    }

    for (row = 0; row < OAF_GC_MAX_OBJECTS; row++)
    {
        collector->edges[row][(size_t)index] = 0;
    }
}

static void mark_reachable(OafGarbageCollector* collector, int index)
{
    size_t child;
    OafGcObject* object;

    object = &collector->objects[(size_t)index];
    if (!object->active || object->marked)
    {
        return;
    }

    object->marked = 1;

    for (child = 0; child < OAF_GC_MAX_OBJECTS; child++)
    {
        if (collector->edges[(size_t)index][child] != 0)
        {
            mark_reachable(collector, (int)child);
        }
    }
}

static int dfs_cycle(const OafGarbageCollector* collector, int index, unsigned char* state)
{
    size_t child;

    state[(size_t)index] = 1;

    for (child = 0; child < OAF_GC_MAX_OBJECTS; child++)
    {
        if (!collector->objects[child].active || collector->edges[(size_t)index][child] == 0)
        {
            continue;
        }

        if (state[child] == 1)
        {
            return 1;
        }

        if (state[child] == 0 && dfs_cycle(collector, (int)child, state))
        {
            return 1;
        }
    }

    state[(size_t)index] = 2;
    return 0;
}

int oaf_gc_init(OafGarbageCollector* collector, OafAllocator* allocator, int enabled)
{
    size_t index;
    size_t edge_index;

    if (collector == NULL || allocator == NULL)
    {
        return 0;
    }

    collector->allocator = allocator;
    collector->active_count = 0;
    collector->managed_bytes = 0;
    collector->enabled = enabled != 0;

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        collector->objects[index].pointer = NULL;
        collector->objects[index].size = 0;
        collector->objects[index].external_refs = 0;
        collector->objects[index].marked = 0;
        collector->objects[index].active = 0;

        for (edge_index = 0; edge_index < OAF_GC_MAX_OBJECTS; edge_index++)
        {
            collector->edges[index][edge_index] = 0;
        }
    }

    return 1;
}

void oaf_gc_destroy(OafGarbageCollector* collector)
{
    size_t index;

    if (collector == NULL || collector->allocator == NULL)
    {
        return;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        OafGcObject* object = &collector->objects[index];

        if (!object->active)
        {
            continue;
        }

        oaf_allocator_free(collector->allocator, object->pointer);
        object->pointer = NULL;
        object->size = 0;
        object->external_refs = 0;
        object->marked = 0;
        object->active = 0;
    }

    collector->active_count = 0;
    collector->managed_bytes = 0;
}

void oaf_gc_set_enabled(OafGarbageCollector* collector, int enabled)
{
    if (collector == NULL)
    {
        return;
    }

    collector->enabled = enabled != 0;
}

void* oaf_gc_alloc(OafGarbageCollector* collector, size_t size, size_t alignment)
{
    int index;
    void* pointer;
    OafGcObject* object;

    if (collector == NULL || collector->allocator == NULL || !collector->enabled)
    {
        return NULL;
    }

    index = find_free_index(collector);
    if (index < 0)
    {
        return NULL;
    }

    pointer = oaf_allocator_alloc(collector->allocator, size, alignment);
    if (pointer == NULL)
    {
        return NULL;
    }

    object = &collector->objects[(size_t)index];
    object->pointer = pointer;
    object->size = size;
    object->external_refs = 0;
    object->marked = 0;
    object->active = 1;
    clear_edges_for_index(collector, index);
    collector->active_count++;
    collector->managed_bytes += size;
    return pointer;
}

int oaf_gc_retain(OafGarbageCollector* collector, void* pointer)
{
    int index = find_object_index(collector, pointer);
    if (index < 0)
    {
        return 0;
    }

    collector->objects[(size_t)index].external_refs++;
    return 1;
}

int oaf_gc_release(OafGarbageCollector* collector, void* pointer)
{
    int index = find_object_index(collector, pointer);
    OafGcObject* object;

    if (index < 0)
    {
        return 0;
    }

    object = &collector->objects[(size_t)index];
    if (object->external_refs == 0)
    {
        return 0;
    }

    object->external_refs--;
    return 1;
}

int oaf_gc_add_reference(OafGarbageCollector* collector, void* from, void* to)
{
    int from_index = find_object_index(collector, from);
    int to_index = find_object_index(collector, to);

    if (from_index < 0 || to_index < 0)
    {
        return 0;
    }

    collector->edges[(size_t)from_index][(size_t)to_index] = 1;
    return 1;
}

int oaf_gc_remove_reference(OafGarbageCollector* collector, void* from, void* to)
{
    int from_index = find_object_index(collector, from);
    int to_index = find_object_index(collector, to);

    if (from_index < 0 || to_index < 0)
    {
        return 0;
    }

    collector->edges[(size_t)from_index][(size_t)to_index] = 0;
    return 1;
}

size_t oaf_gc_collect(OafGarbageCollector* collector)
{
    size_t index;
    size_t collected = 0;

    if (collector == NULL || !collector->enabled)
    {
        return 0;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        collector->objects[index].marked = 0;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        const OafGcObject* object = &collector->objects[index];
        if (object->active && object->external_refs > 0)
        {
            mark_reachable(collector, (int)index);
        }
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        OafGcObject* object = &collector->objects[index];

        if (!object->active || object->marked)
        {
            continue;
        }

        oaf_allocator_free(collector->allocator, object->pointer);
        clear_edges_for_index(collector, (int)index);
        if (collector->managed_bytes >= object->size)
        {
            collector->managed_bytes -= object->size;
        }
        else
        {
            collector->managed_bytes = 0;
        }

        object->pointer = NULL;
        object->size = 0;
        object->external_refs = 0;
        object->marked = 0;
        object->active = 0;

        if (collector->active_count > 0)
        {
            collector->active_count--;
        }
        collected++;
    }

    return collected;
}

int oaf_gc_detect_cycles(const OafGarbageCollector* collector)
{
    unsigned char state[OAF_GC_MAX_OBJECTS];
    size_t index;

    if (collector == NULL)
    {
        return 0;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        state[index] = 0;
    }

    for (index = 0; index < OAF_GC_MAX_OBJECTS; index++)
    {
        if (!collector->objects[index].active || state[index] != 0)
        {
            continue;
        }

        if (dfs_cycle(collector, (int)index, state))
        {
            return 1;
        }
    }

    return 0;
}

size_t oaf_gc_object_count(const OafGarbageCollector* collector)
{
    if (collector == NULL)
    {
        return 0;
    }

    return collector->active_count;
}

size_t oaf_gc_managed_bytes(const OafGarbageCollector* collector)
{
    if (collector == NULL)
    {
        return 0;
    }

    return collector->managed_bytes;
}
