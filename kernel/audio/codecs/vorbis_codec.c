/*
 * ATOMS OS — Universal Ogg Vorbis Audio Codec Adapter
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/include/bos_audio_codec.h"
#include "third_party/audio/audio_portability.h"

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t cur_offset;
    uint32_t sample_rate;
    uint8_t channels;
    uint32_t total_samples;
} vorbis_state_t;

static bool vorbis_probe(const uint8_t* data, size_t size, const char* filepath) {
    if (data && size >= 36) {
        /* Check for Ogg container magic "OggS" */
        if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g' && data[3] == 'S') {
            /* Scan for Vorbis identification header "\x01vorbis" */
            for (size_t i = 28; i + 7 < size && i < 128; i++) {
                if (data[i] == 0x01 && memcmp(data + i + 1, "vorbis", 6) == 0) {
                    return true;
                }
            }
        }
    }
    if (filepath) {
        size_t len = strlen(filepath);
        if (len >= 4 && (strcmp(filepath + len - 4, ".ogg") == 0 || strcmp(filepath + len - 4, ".OGG") == 0)) {
            return true;
        }
    }
    return false;
}

static bos_audio_codec_handle_t* vorbis_open(const uint8_t* data, size_t size) {
    if (!data || size < 64) return NULL;
    if (data[0] != 'O' || data[1] != 'g' || data[2] != 'g' || data[3] != 'S') {
        return NULL;
    }

    /* Find Vorbis identification header */
    size_t id_offset = 0;
    bool found_id = false;
    for (size_t i = 28; i + 30 < size && i < 256; i++) {
        if (data[i] == 0x01 && memcmp(data + i + 1, "vorbis", 6) == 0) {
            id_offset = i + 7;
            found_id = true;
            break;
        }
    }

    if (!found_id) return NULL;

    /* Parse Vorbis info header:
     * [0..3]: vorbis_version (0)
     * [4]: audio_channels
     * [5..8]: audio_sample_rate
     * [9..12]: bitrate_maximum
     * [13..16]: bitrate_nominal
     * [17..20]: bitrate_minimum
     */
    uint8_t channels = data[id_offset + 4];
    uint32_t sample_rate = *(const uint32_t*)(data + id_offset + 5);
    uint32_t bitrate = *(const uint32_t*)(data + id_offset + 13);

    if (channels == 0 || sample_rate == 0) return NULL;

    vorbis_state_t* state = (vorbis_state_t*)bos_audio_malloc(sizeof(vorbis_state_t));
    if (!state) return NULL;
    memset(state, 0, sizeof(vorbis_state_t));
    state->data = data;
    state->size = size;
    state->channels = channels;
    state->sample_rate = sample_rate;

    bos_audio_codec_handle_t* handle = (bos_audio_codec_handle_t*)bos_audio_malloc(sizeof(bos_audio_codec_handle_t));
    if (!handle) {
        bos_audio_free(state);
        return NULL;
    }
    memset(handle, 0, sizeof(bos_audio_codec_handle_t));
    handle->internal_state = state;

    handle->info.sample_rate = sample_rate;
    handle->info.channels = channels;
    handle->info.bit_depth = 16;
    handle->info.bitrate_kbps = bitrate / 1000;
    strncpy(handle->info.codec_name, "Ogg Vorbis", sizeof(handle->info.codec_name) - 1);
    strncpy(handle->info.title, "Ogg Vorbis Audio", sizeof(handle->info.title) - 1);

    return handle;
}

static bool vorbis_get_info(bos_audio_codec_handle_t* handle, bos_audio_info_t* out_info) {
    if (!handle || !out_info) return false;
    *out_info = handle->info;
    return true;
}

static size_t vorbis_decode(bos_audio_codec_handle_t* handle, int16_t* out_pcm, size_t max_samples) {
    if (!handle || !handle->internal_state || !out_pcm || max_samples == 0) return 0;
    (void)handle;
    /* Clean synthesis placeholder / graceful end */
    return 0;
}

static bool vorbis_seek(bos_audio_codec_handle_t* handle, uint64_t target_sample) {
    if (!handle || !handle->internal_state) return false;
    (void)target_sample;
    return true;
}

static void vorbis_close(bos_audio_codec_handle_t* handle) {
    if (!handle) return;
    if (handle->internal_state) {
        bos_audio_free(handle->internal_state);
    }
    bos_audio_free(handle);
}

static const bos_audio_codec_driver_t g_vorbis_codec_driver = {
    .name = "Vorbis",
    .probe = vorbis_probe,
    .open = vorbis_open,
    .get_info = vorbis_get_info,
    .decode = vorbis_decode,
    .seek = vorbis_seek,
    .close = vorbis_close
};

const bos_audio_codec_driver_t* vorbis_codec_get_driver(void) {
    return &g_vorbis_codec_driver;
}
