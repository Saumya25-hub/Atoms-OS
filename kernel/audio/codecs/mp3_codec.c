/*
 * ATOMS OS — Universal MP3 Audio Codec (minimp3 backend)
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/include/bos_audio_codec.h"
#include "third_party/audio/audio_portability.h"

#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "third_party/audio/mp3/include/minimp3.h"

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t cur_offset;
    size_t audio_start_offset;
    mp3dec_t dec;
    mp3dec_frame_info_t last_info;
    int16_t leftover_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    size_t leftover_samples;
    size_t leftover_read_pos;
} mp3_state_t;

/* ID3 Tag Parser */
static void parse_id3v2(const uint8_t* data, size_t size, size_t* out_audio_offset, bos_audio_info_t* info) {
    if (!data || size < 10) return;
    if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
        /* Syncsafe integer for header size */
        uint32_t tag_size = ((uint32_t)(data[6] & 0x7F) << 21) |
                            ((uint32_t)(data[7] & 0x7F) << 14) |
                            ((uint32_t)(data[8] & 0x7F) << 7)  |
                            ((uint32_t)(data[9] & 0x7F));
        size_t full_tag_size = 10 + tag_size;
        if (full_tag_size > size) full_tag_size = size;
        if (out_audio_offset) *out_audio_offset = full_tag_size;

        /* Parse ID3v2.3/ID3v2.4 frames */
        size_t f_offset = 10;
        while (f_offset + 10 < full_tag_size) {
            char fid[5];
            memcpy(fid, data + f_offset, 4);
            fid[4] = '\0';
            if (fid[0] == 0) break; // Padding reached

            uint32_t f_size = ((uint32_t)data[f_offset + 4] << 24) |
                              ((uint32_t)data[f_offset + 5] << 16) |
                              ((uint32_t)data[f_offset + 6] << 8)  |
                              ((uint32_t)data[f_offset + 7]);
            f_offset += 10;
            if (f_offset + f_size > full_tag_size) break;

            if (f_size > 1 && data[f_offset] == 0) { // ISO-8859-1 string
                size_t copy_len = (f_size - 1 < BOS_AUDIO_METADATA_MAX_STR - 1) ? f_size - 1 : BOS_AUDIO_METADATA_MAX_STR - 1;
                if (strcmp(fid, "TIT2") == 0) {
                    memcpy(info->title, data + f_offset + 1, copy_len);
                    info->title[copy_len] = '\0';
                } else if (strcmp(fid, "TPE1") == 0) {
                    memcpy(info->artist, data + f_offset + 1, copy_len);
                    info->artist[copy_len] = '\0';
                } else if (strcmp(fid, "TALB") == 0) {
                    memcpy(info->album, data + f_offset + 1, copy_len);
                    info->album[copy_len] = '\0';
                }
            }
            f_offset += f_size;
        }
    } else {
        if (out_audio_offset) *out_audio_offset = 0;
    }
}

static bool mp3_probe(const uint8_t* data, size_t size, const char* filepath) {
    if (data && size >= 3) {
        if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
            return true;
        }
        /* Look for MPEG sync word (0xFFE0 mask) in the first 4KB */
        size_t scan_limit = (size < 4096) ? size : 4096;
        for (size_t i = 0; i + 1 < scan_limit; i++) {
            if (data[i] == 0xFF && (data[i+1] & 0xE0) == 0xE0) {
                return true;
            }
        }
    }
    if (filepath) {
        size_t len = strlen(filepath);
        if (len >= 4 && (strcmp(filepath + len - 4, ".mp3") == 0 || strcmp(filepath + len - 4, ".MP3") == 0)) {
            return true;
        }
    }
    return false;
}

static bos_audio_codec_handle_t* mp3_open(const uint8_t* data, size_t size) {
    if (!data || size < 32) return NULL;

    mp3_state_t* state = (mp3_state_t*)bos_audio_malloc(sizeof(mp3_state_t));
    if (!state) return NULL;
    memset(state, 0, sizeof(mp3_state_t));

    state->data = data;
    state->size = size;
    mp3dec_init(&state->dec);

    bos_audio_codec_handle_t* handle = (bos_audio_codec_handle_t*)bos_audio_malloc(sizeof(bos_audio_codec_handle_t));
    if (!handle) {
        bos_audio_free(state);
        return NULL;
    }
    memset(handle, 0, sizeof(bos_audio_codec_handle_t));
    handle->internal_state = state;

    /* Parse ID3 tag if present */
    parse_id3v2(data, size, &state->audio_start_offset, &handle->info);
    state->cur_offset = state->audio_start_offset;

    /* Decode first frame to probe stream format */
    int16_t probe_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_frame_info_t info;
    int samples = mp3dec_decode_frame(&state->dec, state->data + state->cur_offset,
                                     (int)(state->size - state->cur_offset),
                                     probe_pcm, &info);

    if (samples <= 0 || info.frame_bytes <= 0) {
        /* Try scanning ahead up to 16KB for the first valid sync frame */
        size_t scan = state->cur_offset;
        bool found = false;
        while (scan + 4 < state->size && scan < state->cur_offset + 16384) {
            if (state->data[scan] == 0xFF && (state->data[scan+1] & 0xE0) == 0xE0) {
                samples = mp3dec_decode_frame(&state->dec, state->data + scan,
                                             (int)(state->size - scan),
                                             probe_pcm, &info);
                if (samples > 0 && info.frame_bytes > 0) {
                    state->cur_offset = scan;
                    found = true;
                    break;
                }
            }
            scan++;
        }
        if (!found) {
            bos_audio_free(state);
            bos_audio_free(handle);
            return NULL;
        }
    }

    state->last_info = info;
    handle->info.sample_rate = info.hz;
    handle->info.channels = (uint8_t)info.channels;
    handle->info.bit_depth = 16;
    handle->info.bitrate_kbps = info.bitrate_kbps;
    strncpy(handle->info.codec_name, "MP3 (Layer III)", sizeof(handle->info.codec_name) - 1);
    if (handle->info.title[0] == '\0') {
        strncpy(handle->info.title, "MPEG Layer-3 Audio", sizeof(handle->info.title) - 1);
    }

    /* Estimate duration from bitrate and file size */
    if (info.bitrate_kbps > 0) {
        size_t audio_bytes = (state->size > state->audio_start_offset) ? (state->size - state->audio_start_offset) : state->size;
        handle->info.duration_ms = (uint32_t)((audio_bytes * 8ULL) / info.bitrate_kbps);
        handle->info.total_samples = ((uint64_t)handle->info.duration_ms * info.hz) / 1000ULL;
    }

    /* Buffer the probed first frame */
    size_t probe_samples = samples * info.channels;
    memcpy(state->leftover_pcm, probe_pcm, probe_samples * sizeof(int16_t));
    state->leftover_samples = probe_samples;
    state->leftover_read_pos = 0;
    state->cur_offset += info.frame_bytes;

    return handle;
}

static bool mp3_get_info(bos_audio_codec_handle_t* handle, bos_audio_info_t* out_info) {
    if (!handle || !out_info) return false;
    *out_info = handle->info;
    return true;
}

static size_t mp3_decode(bos_audio_codec_handle_t* handle, int16_t* out_pcm, size_t max_samples) {
    if (!handle || !handle->internal_state || !out_pcm || max_samples == 0) return 0;
    mp3_state_t* s = (mp3_state_t*)handle->internal_state;

    size_t samples_written = 0;

    /* First drain leftover samples from previous frame */
    if (s->leftover_samples > 0) {
        size_t avail = s->leftover_samples - s->leftover_read_pos;
        size_t take = (avail < max_samples) ? avail : max_samples;
        memcpy(out_pcm, s->leftover_pcm + s->leftover_read_pos, take * sizeof(int16_t));
        s->leftover_read_pos += take;
        samples_written += take;

        if (s->leftover_read_pos >= s->leftover_samples) {
            s->leftover_samples = 0;
            s->leftover_read_pos = 0;
        }

        if (samples_written == max_samples) {
            return samples_written;
        }
    }

    /* Decode subsequent frames */
    while (samples_written < max_samples && s->cur_offset < s->size) {
        int16_t frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        mp3dec_frame_info_t info;
        int samples = mp3dec_decode_frame(&s->dec, s->data + s->cur_offset,
                                         (int)(s->size - s->cur_offset),
                                         frame_pcm, &info);

        if (samples <= 0 || info.frame_bytes <= 0) {
            /* Try skipping corrupt byte */
            s->cur_offset++;
            continue;
        }

        s->cur_offset += info.frame_bytes;
        s->last_info = info;

        size_t total_frame_samples = samples * info.channels;
        size_t needed = max_samples - samples_written;

        if (total_frame_samples <= needed) {
            memcpy(out_pcm + samples_written, frame_pcm, total_frame_samples * sizeof(int16_t));
            samples_written += total_frame_samples;
        } else {
            /* Copy what fits, store remainder */
            memcpy(out_pcm + samples_written, frame_pcm, needed * sizeof(int16_t));
            samples_written += needed;

            size_t leftover = total_frame_samples - needed;
            memcpy(s->leftover_pcm, frame_pcm + needed, leftover * sizeof(int16_t));
            s->leftover_samples = leftover;
            s->leftover_read_pos = 0;
            break;
        }
    }

    return samples_written;
}

static bool mp3_seek(bos_audio_codec_handle_t* handle, uint64_t target_sample) {
    if (!handle || !handle->internal_state) return false;
    mp3_state_t* s = (mp3_state_t*)handle->internal_state;

    if (handle->info.sample_rate == 0 || handle->info.bitrate_kbps == 0) return false;

    uint64_t target_ms = (target_sample * 1000ULL) / handle->info.sample_rate;
    uint64_t target_byte = s->audio_start_offset + ((target_ms * handle->info.bitrate_kbps) / 8ULL);
    if (target_byte > s->size) target_byte = s->size;

    /* Resync to next frame sync */
    size_t scan = (size_t)target_byte;
    while (scan + 1 < s->size) {
        if (s->data[scan] == 0xFF && (s->data[scan+1] & 0xE0) == 0xE0) {
            break;
        }
        scan++;
    }

    s->cur_offset = scan;
    s->leftover_samples = 0;
    s->leftover_read_pos = 0;
    mp3dec_init(&s->dec);
    return true;
}

static void mp3_close(bos_audio_codec_handle_t* handle) {
    if (!handle) return;
    if (handle->internal_state) {
        bos_audio_free(handle->internal_state);
    }
    bos_audio_free(handle);
}

static const bos_audio_codec_driver_t g_mp3_codec_driver = {
    .name = "MP3",
    .probe = mp3_probe,
    .open = mp3_open,
    .get_info = mp3_get_info,
    .decode = mp3_decode,
    .seek = mp3_seek,
    .close = mp3_close
};

const bos_audio_codec_driver_t* mp3_codec_get_driver(void) {
    return &g_mp3_codec_driver;
}
