/*
 * BOSPECTRA V3 — Resource Manager Implementation
 * kernel/media/bospectra/manager/resource_manager.c
 */

#include "resource_manager.h"
#include "../memory/bospectra_memory.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_TRACKED_RESOURCES 128U

static BOSPECTRA_ResourceRecord g_tracked_resources[BOSPECTRA_MAX_TRACKED_RESOURCES];
static bool g_res_manager_initialized = false;
static uint32_t g_next_res_id = 1;

void bospectra_resource_manager_init(void) {
    memset(g_tracked_resources, 0, sizeof(g_tracked_resources));
    g_res_manager_initialized = true;
    g_next_res_id = 1;
    bospectra_log("RESOURCE_MANAGER", "BOSPECTRA V3 Resource Manager initialized.");
}

void bospectra_resource_manager_shutdown(void) {
    bospectra_resource_dump_leaks();
    memset(g_tracked_resources, 0, sizeof(g_tracked_resources));
    g_res_manager_initialized = false;
}

bospectra_error_t bospectra_resource_track_alloc(void* ptr, size_t size, bospectra_resource_type_t type, const char* owner_name, uint32_t* out_id) {
    if (!g_res_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!ptr) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (!g_tracked_resources[i].is_active) {
            g_tracked_resources[i].id = g_next_res_id++;
            g_tracked_resources[i].type = type;
            g_tracked_resources[i].ptr = ptr;
            g_tracked_resources[i].size = size;
            g_tracked_resources[i].owner_name = owner_name ? owner_name : "UnknownOwner";
            g_tracked_resources[i].ref_count = 1;
            g_tracked_resources[i].is_active = true;

            if (out_id) *out_id = g_tracked_resources[i].id;
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_resource_track_free(void* ptr) {
    if (!g_res_manager_initialized || !ptr) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (g_tracked_resources[i].is_active && g_tracked_resources[i].ptr == ptr) {
            memset(&g_tracked_resources[i], 0, sizeof(BOSPECTRA_ResourceRecord));
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_HANDLE_INVALID;
}

bospectra_error_t bospectra_resource_ref(void* ptr) {
    if (!g_res_manager_initialized || !ptr) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (g_tracked_resources[i].is_active && g_tracked_resources[i].ptr == ptr) {
            g_tracked_resources[i].ref_count++;
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_HANDLE_INVALID;
}

bospectra_error_t bospectra_resource_unref(void* ptr) {
    if (!g_res_manager_initialized || !ptr) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (g_tracked_resources[i].is_active && g_tracked_resources[i].ptr == ptr) {
            if (g_tracked_resources[i].ref_count > 0) {
                g_tracked_resources[i].ref_count--;
            }
            return BOSPECTRA_SUCCESS;
        }
    }
    return BOSPECTRA_ERR_HANDLE_INVALID;
}

void bospectra_resource_get_stats(BOSPECTRA_ResourceStats* out_stats) {
    if (!out_stats) return;
    memset(out_stats, 0, sizeof(BOSPECTRA_ResourceStats));

    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (g_tracked_resources[i].is_active) {
            out_stats->active_allocations++;
            out_stats->total_bytes_allocated += g_tracked_resources[i].size;
        }
    }
}

void bospectra_resource_dump_leaks(void) {
    uint32_t leak_count = 0;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_TRACKED_RESOURCES; i++) {
        if (g_tracked_resources[i].is_active) {
            leak_count++;
            bospectra_trace_str("RESOURCE LEAK DETECTED", g_tracked_resources[i].owner_name);
            bospectra_trace_u32("Resource ID", g_tracked_resources[i].id);
            bospectra_trace_hex("Resource Pointer", (uint64_t)(uintptr_t)g_tracked_resources[i].ptr);
            bospectra_trace_u32("Size Bytes", (uint32_t)g_tracked_resources[i].size);
        }
    }
    if (leak_count == 0) {
        bospectra_log("RESOURCE_MANAGER", "Clean Shutdown: 0 Resource Leaks Detected.");
    }
}
