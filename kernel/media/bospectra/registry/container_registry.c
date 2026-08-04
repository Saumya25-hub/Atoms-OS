/*
 * BOSPECTRA V3 — Dynamic Container Registry Implementation
 * kernel/media/bospectra/registry/container_registry.c
 */

#include "container_registry.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_ContainerDriverRecord g_container_records[BOSPECTRA_MAX_CONTAINER_DRIVERS];
static uint32_t g_container_record_count = 0;
static bool g_container_reg_initialized = false;

void bospectra_v3_container_registry_init(void) {
    memset(g_container_records, 0, sizeof(g_container_records));
    g_container_record_count = 0;
    g_container_reg_initialized = true;
    bospectra_log("CONTAINER_REGISTRY", "BOSPECTRA V3 Container Registry Initialized.");
}

void bospectra_v3_container_registry_shutdown(void) {
    memset(g_container_records, 0, sizeof(g_container_records));
    g_container_record_count = 0;
    g_container_reg_initialized = false;
}

bospectra_error_t bospectra_v3_container_register(const BOSPECTRA_ContainerDriverRecord* driver) {
    if (!g_container_reg_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!driver || !driver->format_name || !driver->probe || !driver->open) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    /* Reject duplicates */
    for (uint32_t i = 0; i < g_container_record_count; i++) {
        if (strcmp(g_container_records[i].format_name, driver->format_name) == 0) {
            return BOSPECTRA_ERR_STREAM_EXISTS;
        }
    }

    if (g_container_record_count >= BOSPECTRA_MAX_CONTAINER_DRIVERS) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_container_records[g_container_record_count++] = *driver;
    bospectra_log("CONTAINER_REGISTRY", driver->format_name);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_ContainerDriverRecord* bospectra_v3_container_find_by_name(const char* name) {
    if (!g_container_reg_initialized || !name) return NULL;

    for (uint32_t i = 0; i < g_container_record_count; i++) {
        if (strcmp(g_container_records[i].format_name, name) == 0) {
            return &g_container_records[i];
        }
    }
    return NULL;
}

uint32_t bospectra_v3_container_get_registered(const BOSPECTRA_ContainerDriverRecord** out_drivers, uint32_t max_count) {
    if (!g_container_reg_initialized || !out_drivers) return 0;
    uint32_t count = (g_container_record_count < max_count) ? g_container_record_count : max_count;
    for (uint32_t i = 0; i < count; i++) {
        out_drivers[i] = &g_container_records[i];
    }
    return count;
}
