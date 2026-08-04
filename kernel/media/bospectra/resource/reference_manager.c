/*
 * BOSPECTRA V3 — Reference Counting Implementation
 * kernel/media/bospectra/resource/reference_manager.c
 */

#include "reference_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_REF_CONTROL_BLOCKS 256

static BOSPECTRA_ReferenceControlBlock g_ref_table[BOSPECTRA_MAX_REF_CONTROL_BLOCKS];
static uint32_t                        g_ref_errors = 0;
static bool                            g_ref_mgr_initialized = false;

void bospectra_reference_manager_init(void) {
    memset(g_ref_table, 0, sizeof(g_ref_table));
    g_ref_errors = 0;
    g_ref_mgr_initialized = true;
    bospectra_log("REF_MGR", "BOSPECTRA V3 Reference Manager Initialized.");
}

void bospectra_reference_manager_shutdown(void) {
    g_ref_mgr_initialized = false;
}

bospectra_error_t bospectra_ref_register(uint32_t res_id) {
    if (!g_ref_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_REF_CONTROL_BLOCKS; i++) {
        if (!g_ref_table[i].is_active) {
            g_ref_table[i].resource_id = res_id;
            g_ref_table[i].ref_count = 1;
            g_ref_table[i].is_active = true;
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_ref_add(uint32_t res_id) {
    if (!g_ref_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_REF_CONTROL_BLOCKS; i++) {
        if (g_ref_table[i].is_active && g_ref_table[i].resource_id == res_id) {
            g_ref_table[i].ref_count++;
            return BOSPECTRA_SUCCESS;
        }
    }
    g_ref_errors++;
    bospectra_log("REF_MGR", "Ref Add Error: Invalid Resource ID!");
    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}

bospectra_error_t bospectra_ref_release(uint32_t res_id, uint32_t* out_remaining_refs) {
    if (!g_ref_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_REF_CONTROL_BLOCKS; i++) {
        if (g_ref_table[i].is_active && g_ref_table[i].resource_id == res_id) {
            if (g_ref_table[i].ref_count == 0) {
                g_ref_errors++;
                bospectra_log("REF_MGR", "Ref Release Error: Double Release Detected!");
                return BOSPECTRA_ERR_STATE_INVALID;
            }
            g_ref_table[i].ref_count--;
            if (out_remaining_refs) *out_remaining_refs = g_ref_table[i].ref_count;
            if (g_ref_table[i].ref_count == 0) {
                g_ref_table[i].is_active = false;
            }
            return BOSPECTRA_SUCCESS;
        }
    }
    g_ref_errors++;
    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}

uint32_t bospectra_ref_get_count(uint32_t res_id) {
    if (!g_ref_mgr_initialized) return 0;
    for (size_t i = 0; i < BOSPECTRA_MAX_REF_CONTROL_BLOCKS; i++) {
        if (g_ref_table[i].is_active && g_ref_table[i].resource_id == res_id) {
            return g_ref_table[i].ref_count;
        }
    }
    return 0;
}

uint32_t bospectra_ref_get_errors_count(void) {
    return g_ref_errors;
}
