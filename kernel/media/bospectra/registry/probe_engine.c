/*
 * BOSPECTRA V3 — Dynamic Probe Engine Implementation
 * kernel/media/bospectra/registry/probe_engine.c
 */

#include "probe_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_PROBE_BUFFER_SIZE 512U

static bool g_probe_engine_initialized = false;

void bospectra_v3_probe_engine_init(void) {
    g_probe_engine_initialized = true;
    bospectra_log("PROBE_ENGINE", "BOSPECTRA V3 Dynamic Probe Engine Initialized.");
}

void bospectra_v3_probe_engine_shutdown(void) {
    g_probe_engine_initialized = false;
}

bospectra_error_t bospectra_v3_probe_file(bospectra_file_id_t file_id, BOSPECTRA_ProbeResult* out_result) {
    if (!g_probe_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (file_id == 0 || !out_result) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    memset(out_result, 0, sizeof(BOSPECTRA_ProbeResult));

    uint8_t header_buf[BOSPECTRA_PROBE_BUFFER_SIZE];
    uint32_t bytes_read = 0;

    if (bospectra_file_seek(file_id, 0) != BOSPECTRA_SUCCESS) return BOSPECTRA_ERR_FILE_READ_FAILED;
    if (bospectra_file_read(file_id, header_buf, sizeof(header_buf), &bytes_read) != BOSPECTRA_SUCCESS || bytes_read == 0) {
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }
    out_result->probed_bytes = bytes_read;

    const BOSPECTRA_ContainerDriverRecord* registered_drivers[BOSPECTRA_MAX_CONTAINER_DRIVERS];
    uint32_t count = bospectra_v3_container_get_registered(registered_drivers, BOSPECTRA_MAX_CONTAINER_DRIVERS);

    const BOSPECTRA_ContainerDriverRecord* best_driver = NULL;
    int highest_score = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (registered_drivers[i] && registered_drivers[i]->probe) {
            int score = registered_drivers[i]->probe(file_id, header_buf, bytes_read);
            bospectra_trace_str("Probe Driver Evaluation", registered_drivers[i]->format_name);
            bospectra_trace_u32("Probe Score", (uint32_t)score);

            if (score > highest_score) {
                highest_score = score;
                best_driver = registered_drivers[i];
            }
        }
    }

    /* Reset seek offset after probing */
    bospectra_file_seek(file_id, 0);

    if (highest_score >= 50 && best_driver) {
        out_result->selected_driver = best_driver;
        out_result->highest_score = highest_score;
        bospectra_trace_str("Selected Container Driver", best_driver->format_name);
        bospectra_trace_u32("Winning Match Score", (uint32_t)highest_score);
        return BOSPECTRA_SUCCESS;
    }

    return BOSPECTRA_ERR_STREAM_NOT_FOUND;
}
