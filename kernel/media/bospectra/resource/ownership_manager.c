/*
 * BOSPECTRA V3 — Production Resource Ownership Implementation
 * kernel/media/bospectra/resource/ownership_manager.c
 */

#include "ownership_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_OWNERSHIP_ENTRIES 256

static BOSPECTRA_OwnershipEntry g_ownership_table[BOSPECTRA_MAX_OWNERSHIP_ENTRIES];
static uint32_t                 g_ownership_violations = 0;
static bool                     g_ownership_mgr_initialized = false;

void bospectra_ownership_manager_init(void) {
    memset(g_ownership_table, 0, sizeof(g_ownership_table));
    g_ownership_violations = 0;
    g_ownership_mgr_initialized = true;
    bospectra_log("OWNERSHIP_MGR", "BOSPECTRA V3 Resource Ownership Subsystem Initialized.");
}

void bospectra_ownership_manager_shutdown(void) {
    g_ownership_mgr_initialized = false;
}

bospectra_error_t bospectra_ownership_register(uint32_t res_id, BOSPECTRA_OwnerType owner_type, uint32_t owner_id) {
    if (!g_ownership_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_OWNERSHIP_ENTRIES; i++) {
        if (!g_ownership_table[i].is_valid) {
            g_ownership_table[i].resource_id = res_id;
            g_ownership_table[i].owner_type = owner_type;
            g_ownership_table[i].owner_id = owner_id;
            g_ownership_table[i].creator_id = owner_id;
            g_ownership_table[i].generation_id = 1;
            g_ownership_table[i].is_valid = true;
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_ownership_transfer(uint32_t res_id, BOSPECTRA_OwnerType new_owner_type, uint32_t new_owner_id) {
    if (!g_ownership_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_OWNERSHIP_ENTRIES; i++) {
        if (g_ownership_table[i].is_valid && g_ownership_table[i].resource_id == res_id) {
            g_ownership_table[i].owner_type = new_owner_type;
            g_ownership_table[i].owner_id = new_owner_id;
            g_ownership_table[i].generation_id++;
            return BOSPECTRA_SUCCESS;
        }
    }
    g_ownership_violations++;
    bospectra_log("OWNERSHIP_MGR", "Ownership Transfer Error: Unregistered Resource ID!");
    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}

bospectra_error_t bospectra_ownership_unregister(uint32_t res_id) {
    if (!g_ownership_mgr_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    for (size_t i = 0; i < BOSPECTRA_MAX_OWNERSHIP_ENTRIES; i++) {
        if (g_ownership_table[i].is_valid && g_ownership_table[i].resource_id == res_id) {
            memset(&g_ownership_table[i], 0, sizeof(BOSPECTRA_OwnershipEntry));
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}

bool bospectra_ownership_verify(uint32_t res_id, BOSPECTRA_OwnerType expected_type, uint32_t expected_id) {
    if (!g_ownership_mgr_initialized) return false;

    for (size_t i = 0; i < BOSPECTRA_MAX_OWNERSHIP_ENTRIES; i++) {
        if (g_ownership_table[i].is_valid && g_ownership_table[i].resource_id == res_id) {
            return (g_ownership_table[i].owner_type == expected_type && g_ownership_table[i].owner_id == expected_id);
        }
    }
    g_ownership_violations++;
    return false;
}

uint32_t bospectra_ownership_get_violations_count(void) {
    return g_ownership_violations;
}
