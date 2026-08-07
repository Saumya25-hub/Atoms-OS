#include "bvmm_handle_table.h"
#include "kernel/core/lib/include/string.h"

static bvmm_handle_table_t g_handle_table;
static bool                g_handle_table_initialized = false;

bvmm_result_t bvmm_handle_table_init(void) {
    if (g_handle_table_initialized) return BVMM_ERR_ALREADY_INITIALIZED;

    memset(&g_handle_table, 0, sizeof(bvmm_handle_table_t));
    g_handle_table.next_generation = 1;
    g_handle_table_initialized = true;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_handle_table_shutdown(void) {
    if (!g_handle_table_initialized) return BVMM_ERR_NOT_INITIALIZED;

    memset(&g_handle_table, 0, sizeof(bvmm_handle_table_t));
    g_handle_table_initialized = false;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_handle_table_insert(bvmm_allocation_t* alloc, bvmm_handle_t* out_handle) {
    if (!alloc || !out_handle) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_handle_table_initialized) return BVMM_ERR_NOT_INITIALIZED;

    int slot = -1;
    for (uint32_t i = 0; i < BVMM_HANDLE_TABLE_MAX_ENTRIES; i++) {
        if (!g_handle_table.entries[i].in_use) {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0) return BVMM_ERR_OUT_OF_MEMORY;

    uint16_t gen = g_handle_table.next_generation++;
    if (g_handle_table.next_generation == 0) g_handle_table.next_generation = 1;

    g_handle_table.entries[slot].allocation    = alloc;
    g_handle_table.entries[slot].generation_id = gen;
    g_handle_table.entries[slot].owner_pid     = alloc->owner_pid;
    g_handle_table.entries[slot].in_use        = true;
    g_handle_table.active_count++;

    bvmm_handle_t packed_handle = BVMM_MAKE_HANDLE(gen, alloc->current_domain, slot);
    alloc->handle = packed_handle;
    alloc->generation_id = gen;

    *out_handle = packed_handle;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_handle_table_lookup(bvmm_handle_t handle, bvmm_allocation_t** out_alloc) {
    if (handle == BVMM_INVALID_HANDLE || !out_alloc) return BVMM_ERR_INVALID_ARGUMENT;
    if (!g_handle_table_initialized) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BVMM_HANDLE_GET_INDEX(handle);
    uint16_t gen = BVMM_HANDLE_GET_GEN(handle);

    if (idx >= BVMM_HANDLE_TABLE_MAX_ENTRIES) return BVMM_ERR_INVALID_HANDLE;

    bvmm_handle_entry_t* entry = &g_handle_table.entries[idx];
    if (!entry->in_use || entry->generation_id != gen || !entry->allocation) {
        return BVMM_ERR_HANDLE_EXPIRED; /* Double free or stale handle detection */
    }

    *out_alloc = entry->allocation;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_handle_table_remove(bvmm_handle_t handle) {
    if (handle == BVMM_INVALID_HANDLE) return BVMM_ERR_INVALID_HANDLE;
    if (!g_handle_table_initialized) return BVMM_ERR_NOT_INITIALIZED;

    uint32_t idx = BVMM_HANDLE_GET_INDEX(handle);
    uint16_t gen = BVMM_HANDLE_GET_GEN(handle);

    if (idx >= BVMM_HANDLE_TABLE_MAX_ENTRIES) return BVMM_ERR_INVALID_HANDLE;

    bvmm_handle_entry_t* entry = &g_handle_table.entries[idx];
    if (!entry->in_use || entry->generation_id != gen) {
        return BVMM_ERR_HANDLE_EXPIRED;
    }

    entry->in_use = false;
    entry->allocation = NULL;
    entry->generation_id = 0;

    if (g_handle_table.active_count > 0) {
        g_handle_table.active_count--;
    }

    return BVMM_SUCCESS;
}
