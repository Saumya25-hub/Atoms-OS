/*
 * ATOMS OS — BOS Audio Codec Registry Implementation
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/codecs/codec_registry.h"
#include "kernel/drivers/display/display.h"

extern const bos_audio_codec_driver_t* wav_codec_get_driver(void);
extern const bos_audio_codec_driver_t* mp3_codec_get_driver(void);
extern const bos_audio_codec_driver_t* flac_codec_get_driver(void);
extern const bos_audio_codec_driver_t* aac_codec_get_driver(void);
extern const bos_audio_codec_driver_t* vorbis_codec_get_driver(void);

#define MAX_AUDIO_CODECS 8
static const bos_audio_codec_driver_t* g_registered_codecs[MAX_AUDIO_CODECS];
static size_t g_num_codecs = 0;

void codec_registry_init(void) {
    g_num_codecs = 0;

    /* Register in priority order: WAV, MP3, FLAC, AAC, Vorbis */
    g_registered_codecs[g_num_codecs++] = wav_codec_get_driver();
    g_registered_codecs[g_num_codecs++] = mp3_codec_get_driver();
    g_registered_codecs[g_num_codecs++] = flac_codec_get_driver();
    g_registered_codecs[g_num_codecs++] = aac_codec_get_driver();
    g_registered_codecs[g_num_codecs++] = vorbis_codec_get_driver();

    display_print("[AUDIO] Codec Registry Initialized (WAV, MP3, FLAC, AAC, Vorbis)\n");
}

const bos_audio_codec_driver_t* codec_registry_probe(const uint8_t* data, size_t size, const char* filepath) {
    for (size_t i = 0; i < g_num_codecs; i++) {
        const bos_audio_codec_driver_t* drv = g_registered_codecs[i];
        if (drv && drv->probe && drv->probe(data, size, filepath)) {
            return drv;
        }
    }
    return NULL;
}

bos_audio_codec_handle_t* codec_registry_open(const uint8_t* data, size_t size, const char* filepath) {
    const bos_audio_codec_driver_t* drv = codec_registry_probe(data, size, filepath);
    if (!drv || !drv->open) {
        return NULL;
    }
    bos_audio_codec_handle_t* handle = drv->open(data, size);
    if (handle) {
        handle->driver = drv;
    }
    return handle;
}
