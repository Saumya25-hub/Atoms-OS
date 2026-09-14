/*
 * ATOMS OS — Universal WAV/PCM Audio Codec
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/include/bos_audio_codec.h"
#include "third_party/audio/audio_portability.h"

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t data_offset;
    size_t data_bytes;
    size_t read_offset; /* offset within data chunk */
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wav_state_t;

static bool wav_probe(const uint8_t* data, size_t size, const char* filepath) {
    if (data && size >= 12) {
        if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
            data[8] == 'W' && data[9] == 'A' && data[10] == 'V' && data[11] == 'E') {
            return true;
        }
    }
    if (filepath) {
        size_t len = strlen(filepath);
        if (len >= 4 && (strcmp(filepath + len - 4, ".wav") == 0 || strcmp(filepath + len - 4, ".WAV") == 0)) {
            return true;
        }
    }
    return false;
}

static bos_audio_codec_handle_t* wav_open(const uint8_t* data, size_t size) {
    if (!data || size < 44) return NULL;
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        return NULL;
    }

    wav_state_t* state = (wav_state_t*)bos_audio_malloc(sizeof(wav_state_t));
    if (!state) return NULL;
    memset(state, 0, sizeof(wav_state_t));
    state->data = data;
    state->size = size;

    /* Parse RIFF Chunks */
    size_t offset = 12;
    bool found_fmt = false;
    bool found_data = false;

    while (offset + 8 <= size) {
        char chunk_id[5];
        memcpy(chunk_id, data + offset, 4);
        chunk_id[4] = '\0';
        uint32_t chunk_size = *(const uint32_t*)(data + offset + 4);
        offset += 8;

        if (offset + chunk_size > size) {
            chunk_size = (uint32_t)(size - offset);
        }

        if (memcmp(chunk_id, "fmt ", 4) == 0 && chunk_size >= 16) {
            state->audio_format    = *(const uint16_t*)(data + offset);
            state->channels        = *(const uint16_t*)(data + offset + 2);
            state->sample_rate    = *(const uint32_t*)(data + offset + 4);
            state->byte_rate      = *(const uint32_t*)(data + offset + 8);
            state->block_align    = *(const uint16_t*)(data + offset + 12);
            state->bits_per_sample = *(const uint16_t*)(data + offset + 14);
            found_fmt = true;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            state->data_offset = offset;
            state->data_bytes  = chunk_size;
            found_data = true;
            break; /* Standard data chunk reached */
        }

        offset += (chunk_size + 1) & ~1; /* Chunks are word-aligned */
    }

    if (!found_fmt || !found_data || state->channels == 0 || state->bits_per_sample == 0) {
        bos_audio_free(state);
        return NULL;
    }

    bos_audio_codec_handle_t* handle = (bos_audio_codec_handle_t*)bos_audio_malloc(sizeof(bos_audio_codec_handle_t));
    if (!handle) {
        bos_audio_free(state);
        return NULL;
    }
    memset(handle, 0, sizeof(bos_audio_codec_handle_t));
    handle->internal_state = state;

    handle->info.sample_rate = state->sample_rate;
    handle->info.channels = (uint8_t)state->channels;
    handle->info.bit_depth = (uint8_t)state->bits_per_sample;
    size_t bpf = state->channels * (state->bits_per_sample / 8);
    handle->info.total_samples = (bpf > 0) ? (state->data_bytes / bpf) : 0;
    if (state->sample_rate > 0) {
        handle->info.duration_ms = (uint32_t)((handle->info.total_samples * 1000ULL) / state->sample_rate);
    }
    handle->info.bitrate_kbps = (state->byte_rate * 8) / 1000;
    strncpy(handle->info.codec_name, "PCM", sizeof(handle->info.codec_name) - 1);
    strncpy(handle->info.title, "WAV Audio Stream", sizeof(handle->info.title) - 1);

    return handle;
}

static bool wav_get_info(bos_audio_codec_handle_t* handle, bos_audio_info_t* out_info) {
    if (!handle || !out_info) return false;
    *out_info = handle->info;
    return true;
}

static size_t wav_decode(bos_audio_codec_handle_t* handle, int16_t* out_pcm, size_t max_samples) {
    if (!handle || !handle->internal_state || !out_pcm || max_samples == 0) return 0;
    wav_state_t* s = (wav_state_t*)handle->internal_state;

    if (s->read_offset >= s->data_bytes) return 0;

    size_t samples_written = 0;
    size_t bytes_left = s->data_bytes - s->read_offset;
    const uint8_t* p = s->data + s->data_offset + s->read_offset;

    if (s->bits_per_sample == 16) {
        size_t available_samples = bytes_left / 2;
        size_t to_write = (available_samples < max_samples) ? available_samples : max_samples;
        const int16_t* src16 = (const int16_t*)p;
        for (size_t i = 0; i < to_write; i++) {
            out_pcm[i] = src16[i];
        }
        samples_written = to_write;
        s->read_offset += to_write * 2;
    } else if (s->bits_per_sample == 8) {
        /* 8-bit unsigned PCM: convert [0..255] to [-32768..32767] */
        size_t available_samples = bytes_left;
        size_t to_write = (available_samples < max_samples) ? available_samples : max_samples;
        for (size_t i = 0; i < to_write; i++) {
            out_pcm[i] = (int16_t)(((int32_t)p[i] - 128) << 8);
        }
        samples_written = to_write;
        s->read_offset += to_write;
    } else if (s->bits_per_sample == 24) {
        /* 24-bit signed PCM (3 bytes per sample) -> scale to 16-bit */
        size_t available_samples = bytes_left / 3;
        size_t to_write = (available_samples < max_samples) ? available_samples : max_samples;
        for (size_t i = 0; i < to_write; i++) {
            int32_t val = (int32_t)(p[i * 3 + 0] | (p[i * 3 + 1] << 8) | (p[i * 3 + 2] << 16));
            if (val & 0x800000) val |= 0xFF000000;
            out_pcm[i] = (int16_t)(val >> 8);
        }
        samples_written = to_write;
        s->read_offset += to_write * 3;
    } else if (s->bits_per_sample == 32) {
        /* 32-bit signed PCM -> scale to 16-bit */
        size_t available_samples = bytes_left / 4;
        size_t to_write = (available_samples < max_samples) ? available_samples : max_samples;
        const int32_t* src32 = (const int32_t*)p;
        for (size_t i = 0; i < to_write; i++) {
            out_pcm[i] = (int16_t)(src32[i] >> 16);
        }
        samples_written = to_write;
        s->read_offset += to_write * 4;
    }

    return samples_written;
}

static bool wav_seek(bos_audio_codec_handle_t* handle, uint64_t target_sample) {
    if (!handle || !handle->internal_state) return false;
    wav_state_t* s = (wav_state_t*)handle->internal_state;

    size_t bpf = s->channels * (s->bits_per_sample / 8);
    size_t target_offset = (size_t)target_sample * bpf;
    if (target_offset > s->data_bytes) target_offset = s->data_bytes;

    s->read_offset = target_offset;
    return true;
}

static void wav_close(bos_audio_codec_handle_t* handle) {
    if (!handle) return;
    if (handle->internal_state) {
        bos_audio_free(handle->internal_state);
    }
    bos_audio_free(handle);
}

static const bos_audio_codec_driver_t g_wav_codec_driver = {
    .name = "WAV/PCM",
    .probe = wav_probe,
    .open = wav_open,
    .get_info = wav_get_info,
    .decode = wav_decode,
    .seek = wav_seek,
    .close = wav_close
};

const bos_audio_codec_driver_t* wav_codec_get_driver(void) {
    return &g_wav_codec_driver;
}
