#include "bospectra_stream.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_StreamDescriptor g_bospectra_streams[BOSPECTRA_MAX_STREAMS];
static uint32_t g_bospectra_active_stream_count = 0;
static bool g_bospectra_stream_initialized = false;

void bospectra_stream_subsystem_init(void) {
    memset(g_bospectra_streams, 0, sizeof(g_bospectra_streams));
    g_bospectra_active_stream_count = 0;
    g_bospectra_stream_initialized = true;
}

void bospectra_stream_subsystem_shutdown(void) {
    memset(g_bospectra_streams, 0, sizeof(g_bospectra_streams));
    g_bospectra_active_stream_count = 0;
    g_bospectra_stream_initialized = false;
}

bospectra_error_t bospectra_stream_register(const BOSPECTRA_StreamDescriptor* desc, bospectra_stream_id_t* out_id) {
    if (!g_bospectra_stream_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!desc || !out_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOSPECTRA_MAX_STREAMS; i++) {
        if (!g_bospectra_streams[i].is_active) {
            g_bospectra_streams[i] = *desc;
            g_bospectra_streams[i].id = i + 1; // 1-indexed IDs
            g_bospectra_streams[i].is_active = true;
            *out_id = g_bospectra_streams[i].id;
            g_bospectra_active_stream_count++;
            return BOSPECTRA_SUCCESS;
        }
    }

    return BOSPECTRA_ERR_BUFFER_OVERFLOW;
}

bospectra_error_t bospectra_stream_unregister(bospectra_stream_id_t stream_id) {
    if (!g_bospectra_stream_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (stream_id == 0 || stream_id > BOSPECTRA_MAX_STREAMS) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    uint32_t idx = stream_id - 1;
    if (!g_bospectra_streams[idx].is_active) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    memset(&g_bospectra_streams[idx], 0, sizeof(BOSPECTRA_StreamDescriptor));
    g_bospectra_active_stream_count--;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_stream_get_descriptor(bospectra_stream_id_t stream_id, BOSPECTRA_StreamDescriptor* out_desc) {
    if (!g_bospectra_stream_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (stream_id == 0 || stream_id > BOSPECTRA_MAX_STREAMS) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    uint32_t idx = stream_id - 1;
    if (!g_bospectra_streams[idx].is_active) return BOSPECTRA_ERR_STREAM_NOT_FOUND;

    *out_desc = g_bospectra_streams[idx];
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_stream_get_count(void) {
    return g_bospectra_active_stream_count;
}
