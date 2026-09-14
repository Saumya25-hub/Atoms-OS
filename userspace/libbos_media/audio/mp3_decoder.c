/*
 * ============================================================================
 * ATOMS OS — Userspace MP3 Audio Decoder Implementation
 * userspace/libbos_media/audio/mp3_decoder.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Powered by minimp3 (CC0 1.0 Universal / Public Domain)
 * ============================================================================
 */

#include "mp3_decoder.h"
#include <stdlib.h>
#include <string.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD
#include "third_party/audio/mp3/include/minimp3.h"

#define MP3_STREAM_BUF_SIZE 16384

struct BOSMp3Decoder {
    BOSMediaStream* stream;
    mp3dec_t        mp3dec;
    uint8_t         buffer[MP3_STREAM_BUF_SIZE];
    int             buf_len;
    int             buf_pos;
    bool            eof_reached;
    int             channels;
    int             hz;
    int             bitrate_kbps;
};

static BOSMp3Decoder s_mp3_dec_instance __attribute__((aligned(16)));

BOSMp3Decoder* bos_mp3_decoder_create(BOSMediaStream* stream) {
    if (!stream) return NULL;
    BOSMp3Decoder* dec = &s_mp3_dec_instance;
    memset(dec, 0, sizeof(BOSMp3Decoder));
    dec->stream = stream;
    mp3dec_init(&dec->mp3dec);
    dec->channels = 2;
    dec->hz = 44100;
    return dec;
}

void bos_mp3_decoder_destroy(BOSMp3Decoder* dec) {
    if (!dec) return;
    dec->stream = NULL;
    dec->buf_len = 0;
    dec->buf_pos = 0;
}

int bos_mp3_decoder_read_frame(BOSMp3Decoder* dec, int16_t* out_pcm, int max_samples,
                               int* out_samples, int* out_channels, int* out_hz) {
    if (!dec || !dec->stream || !out_pcm || max_samples < MINIMP3_MAX_SAMPLES_PER_FRAME) {
        return -1;
    }

    while (1) {
        // Refill buffer if needed
        int remaining = dec->buf_len - dec->buf_pos;
        if (remaining < 4096 && !dec->eof_reached) {
            if (remaining > 0 && dec->buf_pos > 0) {
                memmove(dec->buffer, dec->buffer + dec->buf_pos, remaining);
            }
            dec->buf_pos = 0;
            dec->buf_len = remaining;

            int to_read = MP3_STREAM_BUF_SIZE - remaining;
            int rd = dec->stream->read(dec->stream, dec->buffer + remaining, to_read);
            if (rd > 0) {
                dec->buf_len += rd;
            } else {
                dec->eof_reached = true;
            }
            remaining = dec->buf_len - dec->buf_pos;
        }

        if (remaining <= 0) {
            return 0; // EOF
        }

        mp3dec_frame_info_t info;
        int samples = mp3dec_decode_frame(&dec->mp3dec, dec->buffer + dec->buf_pos, remaining, out_pcm, &info);

        if (info.frame_bytes > 0) {
            dec->buf_pos += info.frame_bytes;
        }

        if (samples > 0) {
            dec->channels = info.channels;
            dec->hz = info.hz;
            dec->bitrate_kbps = info.bitrate_kbps;

            if (out_samples)  *out_samples  = samples * info.channels;
            if (out_channels) *out_channels = info.channels;
            if (out_hz)       *out_hz       = info.hz;
            return samples;
        }

        if (info.frame_bytes == 0) {
            // Skip 1 byte if sync lost
            dec->buf_pos++;
            if (dec->buf_pos >= dec->buf_len && dec->eof_reached) {
                return 0; // EOF
            }
        }
    }
}

int bos_mp3_decoder_get_info(BOSMp3Decoder* dec, int* out_channels, int* out_hz, int* out_bitrate_kbps) {
    if (!dec) return -1;
    if (out_channels)     *out_channels     = dec->channels;
    if (out_hz)           *out_hz           = dec->hz;
    if (out_bitrate_kbps) *out_bitrate_kbps = dec->bitrate_kbps;
    return 0;
}
