/*
 * ATOMS OS — Universal FLAC Audio Codec (dr_flac backend)
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/include/bos_audio_codec.h"
#include "third_party/audio/audio_portability.h"

#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_CRC
#define DRFLAC_MALLOC(sz) bos_audio_malloc(sz)
#define DRFLAC_REALLOC(p, sz) bos_audio_realloc(p, sz)
#define DRFLAC_FREE(p) bos_audio_free(p)
#define DRFLAC_ASSERT(x) ((void)0)

#define DR_FLAC_IMPLEMENTATION
#include "third_party/audio/flac/include/dr_flac.h"

typedef struct {
    drflac* flac;
} flac_state_t;

static bool flac_probe(const uint8_t* data, size_t size, const char* filepath) {
    if (data && size >= 4) {
        if (data[0] == 'f' && data[1] == 'L' && data[2] == 'a' && data[3] == 'C') {
            return true;
        }
    }
    if (filepath) {
        size_t len = strlen(filepath);
        if (len >= 5 && (strcmp(filepath + len - 5, ".flac") == 0 || strcmp(filepath + len - 5, ".FLAC") == 0)) {
            return true;
        }
    }
    return false;
}

static bos_audio_codec_handle_t* flac_open(const uint8_t* data, size_t size) {
    if (!data || size < 4) return NULL;
    if (memcmp(data, "fLaC", 4) != 0) return NULL;

    drflac* pFlac = drflac_open_memory(data, size, NULL);
    if (!pFlac) return NULL;

    flac_state_t* state = (flac_state_t*)bos_audio_malloc(sizeof(flac_state_t));
    if (!state) {
        drflac_close(pFlac);
        return NULL;
    }
    state->flac = pFlac;

    bos_audio_codec_handle_t* handle = (bos_audio_codec_handle_t*)bos_audio_malloc(sizeof(bos_audio_codec_handle_t));
    if (!handle) {
        drflac_close(pFlac);
        bos_audio_free(state);
        return NULL;
    }
    memset(handle, 0, sizeof(bos_audio_codec_handle_t));
    handle->internal_state = state;

    handle->info.sample_rate = pFlac->sampleRate;
    handle->info.channels = (uint8_t)pFlac->channels;
    handle->info.bit_depth = (uint8_t)pFlac->bitsPerSample;
    handle->info.total_samples = pFlac->totalPCMFrameCount;
    if (pFlac->sampleRate > 0) {
        handle->info.duration_ms = (uint32_t)((pFlac->totalPCMFrameCount * 1000ULL) / pFlac->sampleRate);
    }
    if (handle->info.duration_ms > 0) {
        handle->info.bitrate_kbps = (uint32_t)((size * 8ULL) / handle->info.duration_ms);
    }
    strncpy(handle->info.codec_name, "FLAC (Lossless)", sizeof(handle->info.codec_name) - 1);
    strncpy(handle->info.title, "FLAC Audio Stream", sizeof(handle->info.title) - 1);

    return handle;
}

static bool flac_get_info(bos_audio_codec_handle_t* handle, bos_audio_info_t* out_info) {
    if (!handle || !out_info) return false;
    *out_info = handle->info;
    return true;
}

static size_t flac_decode(bos_audio_codec_handle_t* handle, int16_t* out_pcm, size_t max_samples) {
    if (!handle || !handle->internal_state || !out_pcm || max_samples == 0) return 0;
    flac_state_t* s = (flac_state_t*)handle->internal_state;
    if (!s->flac) return 0;

    uint8_t ch = (uint8_t)s->flac->channels;
    if (ch == 0) return 0;

    size_t max_frames = max_samples / ch;
    if (max_frames == 0) return 0;

    drflac_uint64 frames_read = drflac_read_pcm_frames_s16(s->flac, max_frames, out_pcm);
    return (size_t)(frames_read * ch);
}

static bool flac_seek(bos_audio_codec_handle_t* handle, uint64_t target_sample) {
    if (!handle || !handle->internal_state) return false;
    flac_state_t* s = (flac_state_t*)handle->internal_state;
    if (!s->flac) return false;

    return drflac_seek_to_pcm_frame(s->flac, target_sample) == DRFLAC_TRUE;
}

static void flac_close(bos_audio_codec_handle_t* handle) {
    if (!handle) return;
    if (handle->internal_state) {
        flac_state_t* s = (flac_state_t*)handle->internal_state;
        if (s->flac) {
            drflac_close(s->flac);
        }
        bos_audio_free(s);
    }
    bos_audio_free(handle);
}

static const bos_audio_codec_driver_t g_flac_codec_driver = {
    .name = "FLAC",
    .probe = flac_probe,
    .open = flac_open,
    .get_info = flac_get_info,
    .decode = flac_decode,
    .seek = flac_seek,
    .close = flac_close
};

const bos_audio_codec_driver_t* flac_codec_get_driver(void) {
    return &g_flac_codec_driver;
}
