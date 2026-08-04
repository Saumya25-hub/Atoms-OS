#include "container_registry.h"
#include "../../include/bospectra_errors.h"
#include "../../file/bospectra_file.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_DRIVERS 16U

static const BOSPECTRA_ContainerDriver* g_bospectra_drivers[BOSPECTRA_MAX_DRIVERS];
static uint32_t g_bospectra_driver_count = 0;
static bool g_bospectra_registry_initialized = false;

void bospectra_container_registry_init(void) {
    memset(g_bospectra_drivers, 0, sizeof(g_bospectra_drivers));
    g_bospectra_driver_count = 0;
    g_bospectra_registry_initialized = true;
}

void bospectra_container_registry_shutdown(void) {
    memset(g_bospectra_drivers, 0, sizeof(g_bospectra_drivers));
    g_bospectra_driver_count = 0;
    g_bospectra_registry_initialized = false;
}

bospectra_error_t bospectra_container_register_driver(const BOSPECTRA_ContainerDriver* driver) {
    if (!g_bospectra_registry_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!driver || !driver->format_name || !driver->probe || !driver->open) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    if (g_bospectra_driver_count >= BOSPECTRA_MAX_DRIVERS) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_bospectra_drivers[g_bospectra_driver_count++] = driver;
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_ContainerDriver* bospectra_container_find_driver_by_name(const char* name) {
    if (!g_bospectra_registry_initialized || !name) return NULL;

    for (uint32_t i = 0; i < g_bospectra_driver_count; i++) {
        if (g_bospectra_drivers[i] && strcmp(g_bospectra_drivers[i]->format_name, name) == 0) {
            return g_bospectra_drivers[i];
        }
    }
    return NULL;
}

const BOSPECTRA_ContainerDriver* bospectra_container_probe_driver(bospectra_file_id_t file_id) {
    if (!g_bospectra_registry_initialized) return NULL;

    uint8_t header_buffer[512];
    uint32_t bytes_read = 0;

    // Reset offset to 0 and read probing header
    if (bospectra_file_seek(file_id, 0) != BOSPECTRA_SUCCESS) return NULL;
    if (bospectra_file_read(file_id, header_buffer, sizeof(header_buffer), &bytes_read) != BOSPECTRA_SUCCESS) {
        return NULL;
    }
    if (bytes_read == 0) return NULL;

    const BOSPECTRA_ContainerDriver* best_driver = NULL;
    int highest_score = 0;

    for (uint32_t i = 0; i < g_bospectra_driver_count; i++) {
        if (g_bospectra_drivers[i] && g_bospectra_drivers[i]->probe) {
            int score = g_bospectra_drivers[i]->probe(file_id, header_buffer, bytes_read);
            if (score > highest_score) {
                highest_score = score;
                best_driver = g_bospectra_drivers[i];
            }
        }
    }

    // Reset offset to 0 after probing
    bospectra_file_seek(file_id, 0);

    return (highest_score >= 50) ? best_driver : NULL;
}
