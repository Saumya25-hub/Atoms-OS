#include "decoder_registry.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_DECODER_DRIVERS 16U

static const BOSPECTRA_DecoderDriver* g_decoder_drivers[BOSPECTRA_MAX_DECODER_DRIVERS];
static uint32_t g_decoder_driver_count = 0;
static bool g_decoder_registry_initialized = false;

void bospectra_decoder_registry_init(void) {
    memset(g_decoder_drivers, 0, sizeof(g_decoder_drivers));
    g_decoder_driver_count = 0;
    g_decoder_registry_initialized = true;
}

void bospectra_decoder_registry_shutdown(void) {
    memset(g_decoder_drivers, 0, sizeof(g_decoder_drivers));
    g_decoder_driver_count = 0;
    g_decoder_registry_initialized = false;
}

bospectra_error_t bospectra_decoder_register_driver(const BOSPECTRA_DecoderDriver* driver) {
    if (!g_decoder_registry_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!driver || !driver->codec_name || !driver->open || !driver->decode_packet) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    if (g_decoder_driver_count >= BOSPECTRA_MAX_DECODER_DRIVERS) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_decoder_drivers[g_decoder_driver_count++] = driver;
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_DecoderDriver* bospectra_decoder_find_driver(bospectra_codec_id_t codec_id) {
    if (!g_decoder_registry_initialized) return NULL;

    for (uint32_t i = 0; i < g_decoder_driver_count; i++) {
        if (g_decoder_drivers[i] && g_decoder_drivers[i]->codec_id == codec_id) {
            return g_decoder_drivers[i];
        }
    }
    return NULL;
}

const BOSPECTRA_DecoderDriver* bospectra_decoder_find_driver_by_name(const char* codec_name) {
    if (!g_decoder_registry_initialized || !codec_name) return NULL;

    for (uint32_t i = 0; i < g_decoder_driver_count; i++) {
        if (g_decoder_drivers[i] && strcmp(g_decoder_drivers[i]->codec_name, codec_name) == 0) {
            return g_decoder_drivers[i];
        }
    }
    return NULL;
}
