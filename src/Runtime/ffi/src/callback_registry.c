#include <stddef.h>
#include "callback_registry.h"

typedef struct OafFfiTrampolineSlot
{
    int in_use;
    OafFfiCallbackRegistry* registry;
    uint64_t callback_id;
} OafFfiTrampolineSlot;

static OafFfiTrampolineSlot TRAMPOLINE_SLOTS[OAF_FFI_MAX_TRAMPOLINES];

static const OafFfiCallbackEntry* find_entry(const OafFfiCallbackRegistry* registry, uint64_t callback_id)
{
    size_t index;

    if (registry == NULL || callback_id == 0)
    {
        return NULL;
    }

    for (index = 0; index < OAF_FFI_MAX_CALLBACKS; index++)
    {
        const OafFfiCallbackEntry* entry = &registry->entries[index];
        if (entry->active && entry->id == callback_id)
        {
            return entry;
        }
    }

    return NULL;
}

static int64_t invoke_trampoline_slot(size_t slot_index, int64_t argument)
{
    OafFfiValue args[1];
    int success = 0;
    const OafFfiTrampolineSlot* slot;

    if (slot_index >= OAF_FFI_MAX_TRAMPOLINES)
    {
        return 0;
    }

    slot = &TRAMPOLINE_SLOTS[slot_index];
    if (!slot->in_use || slot->registry == NULL)
    {
        return 0;
    }

    oaf_ffi_value_set_i64(&args[0], argument);
    return oaf_ffi_callback_invoke_i64(slot->registry, slot->callback_id, args, 1, &success);
}

static int64_t trampoline_slot_0(int64_t argument) { return invoke_trampoline_slot(0, argument); }
static int64_t trampoline_slot_1(int64_t argument) { return invoke_trampoline_slot(1, argument); }
static int64_t trampoline_slot_2(int64_t argument) { return invoke_trampoline_slot(2, argument); }
static int64_t trampoline_slot_3(int64_t argument) { return invoke_trampoline_slot(3, argument); }
static int64_t trampoline_slot_4(int64_t argument) { return invoke_trampoline_slot(4, argument); }
static int64_t trampoline_slot_5(int64_t argument) { return invoke_trampoline_slot(5, argument); }
static int64_t trampoline_slot_6(int64_t argument) { return invoke_trampoline_slot(6, argument); }
static int64_t trampoline_slot_7(int64_t argument) { return invoke_trampoline_slot(7, argument); }

static OafFfiTrampolineI64I64 trampoline_fn_from_index(size_t index)
{
    switch (index)
    {
        case 0: return trampoline_slot_0;
        case 1: return trampoline_slot_1;
        case 2: return trampoline_slot_2;
        case 3: return trampoline_slot_3;
        case 4: return trampoline_slot_4;
        case 5: return trampoline_slot_5;
        case 6: return trampoline_slot_6;
        case 7: return trampoline_slot_7;
        default: return NULL;
    }
}

void oaf_ffi_callback_registry_init(OafFfiCallbackRegistry* registry)
{
    size_t index;

    if (registry == NULL)
    {
        return;
    }

    registry->next_id = 1;

    for (index = 0; index < OAF_FFI_MAX_CALLBACKS; index++)
    {
        registry->entries[index].id = 0;
        registry->entries[index].proc = NULL;
        registry->entries[index].user_data = NULL;
        registry->entries[index].active = 0;
    }
}

uint64_t oaf_ffi_callback_register(OafFfiCallbackRegistry* registry, OafFfiCallbackProc proc, void* user_data)
{
    size_t index;

    if (registry == NULL || proc == NULL)
    {
        return 0;
    }

    for (index = 0; index < OAF_FFI_MAX_CALLBACKS; index++)
    {
        OafFfiCallbackEntry* entry = &registry->entries[index];
        if (!entry->active)
        {
            entry->id = registry->next_id++;
            entry->proc = proc;
            entry->user_data = user_data;
            entry->active = 1;
            return entry->id;
        }
    }

    return 0;
}

int oaf_ffi_callback_unregister(OafFfiCallbackRegistry* registry, uint64_t callback_id)
{
    size_t index;

    if (registry == NULL || callback_id == 0)
    {
        return 0;
    }

    for (index = 0; index < OAF_FFI_MAX_CALLBACKS; index++)
    {
        OafFfiCallbackEntry* entry = &registry->entries[index];
        if (entry->active && entry->id == callback_id)
        {
            entry->active = 0;
            entry->id = 0;
            entry->proc = NULL;
            entry->user_data = NULL;
            return 1;
        }
    }

    return 0;
}

int64_t oaf_ffi_callback_invoke_i64(
    const OafFfiCallbackRegistry* registry,
    uint64_t callback_id,
    const OafFfiValue* args,
    size_t arg_count,
    int* success)
{
    const OafFfiCallbackEntry* entry = find_entry(registry, callback_id);

    if (success != NULL)
    {
        *success = 0;
    }

    if (entry == NULL || entry->proc == NULL)
    {
        return 0;
    }

    if (success != NULL)
    {
        *success = 1;
    }

    return entry->proc(args, arg_count, entry->user_data);
}

int oaf_ffi_callback_acquire_trampoline_i64_i64(
    OafFfiCallbackRegistry* registry,
    uint64_t callback_id,
    OafFfiTrampolineI64I64* trampoline_out)
{
    size_t index;

    if (registry == NULL || trampoline_out == NULL)
    {
        return 0;
    }

    if (find_entry(registry, callback_id) == NULL)
    {
        return 0;
    }

    for (index = 0; index < OAF_FFI_MAX_TRAMPOLINES; index++)
    {
        OafFfiTrampolineSlot* slot = &TRAMPOLINE_SLOTS[index];
        if (!slot->in_use)
        {
            slot->in_use = 1;
            slot->registry = registry;
            slot->callback_id = callback_id;
            *trampoline_out = trampoline_fn_from_index(index);
            return *trampoline_out != NULL;
        }
    }

    return 0;
}

void oaf_ffi_callback_release_trampoline(OafFfiTrampolineI64I64 trampoline)
{
    size_t index;

    if (trampoline == NULL)
    {
        return;
    }

    for (index = 0; index < OAF_FFI_MAX_TRAMPOLINES; index++)
    {
        if (trampoline_fn_from_index(index) == trampoline)
        {
            TRAMPOLINE_SLOTS[index].in_use = 0;
            TRAMPOLINE_SLOTS[index].registry = NULL;
            TRAMPOLINE_SLOTS[index].callback_id = 0;
            return;
        }
    }
}
