/*
 * BOSPECTRA V3 — Dynamic Codec Registry Implementation
 * kernel/media/bospectra/registry/codec_registry.c
 */

#include "codec_registry.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_CodecDriverRecord g_codec_records[BOSPECTRA_MAX_CODEC_DRIVERS];
static uint32_t g_codec_record_count = 0;
static bool g_codec_reg_initialized = false;

void bospectra_v3_codec_registry_init(void) {
    memset(g_codec_records, 0, sizeof(g_codec_records));
    g_codec_record_count = 0;
    g_codec_reg_initialized = true;
    bospectra_log("CODEC_REGISTRY", "BOSPECTRA V3 Codec Registry Initialized.");
}

void bospectra_v3_codec_registry_shutdown(void) {
    memset(g_codec_records, 0, sizeof(g_codec_records));
    g_codec_record_count = 0;
    g_codec_reg_initialized = false;
}

bospectra_error_t bospectra_v3_codec_register(const BOSPECTRA_CodecDriverRecord* driver) {
    if (!g_codec_reg_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!driver || !driver->codec_name || !driver->open || !driver->decode_packet) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    /* Reject duplicate codec names */
    for (uint32_t i = 0; i < g_codec_record_count; i++) {
        if (strcmp(g_codec_records[i].codec_name, driver->codec_name) == 0) {
            return BOSPECTRA_ERR_STREAM_EXISTS;
        }
    }

    if (g_codec_record_count >= BOSPECTRA_MAX_CODEC_DRIVERS) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_codec_records[g_codec_record_count++] = *driver;
    bospectra_log("CODEC_REGISTRY", driver->codec_name);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_CodecDriverRecord* bospectra_v3_codec_find_by_name(const char* name) {
    if (!g_codec_reg_initialized || !name) return NULL;

    for (uint32_t i = 0; i < g_codec_record_count; i++) {
        if (strcmp(g_codec_records[i].codec_name, name) == 0) {
            return &g_codec_records[i];
        }
    }
    return NULL;
}

const BOSPECTRA_CodecDriverRecord* bospectra_v3_codec_find_by_id(bospectra_codec_id_t codec_id) {
    if (!g_codec_reg_initialized) return NULL;

    for (uint32_t i = 0; i < g_codec_record_count; i++) {
        if (g_codec_records[i].codec_id == codec_id) {
            return &g_codec_records[i];
        }
    }
    return NULL;
}

uint32_t bospectra_v3_codec_get_registered(const BOSPECTRA_CodecDriverRecord** out_drivers, uint32_t max_count) {
    if (!g_codec_reg_initialized || !out_drivers) return 0;
    uint32_t count = (g_codec_record_count < max_count) ? g_codec_record_count : max_count;
    for (uint32_t i = 0; i < count; i++) {
        out_drivers[i] = &g_codec_records[i];
    }
    return count;
}
