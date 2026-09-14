/*
 * ============================================================================
 * ATOMS OS — Userspace Audio Stream Implementation
 * userspace/libbos_media/audio/bos_audio_stream.c
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements BOSAudioStream with format negotiation, bounded FIFO, and Audio HAL.
 * ============================================================================
 */

#include "bos_audio_stream.h"
#include "kernel/audio/api/audio_api.h"
#include <string.h>

extern void display_print(const char* s);

static void audio_print_num(uint64_t val) {
    char buf[24];
    int p = 0;
    if (val == 0) {
        display_print("0");
        return;
    }
    char tmp[24];
    int tp = 0;
    while (val > 0) {
        tmp[tp++] = '0' + (val % 10);
        val /= 10;
    }
    while (tp > 0) {
        buf[p++] = tmp[--tp];
    }
    buf[p] = '\0';
    display_print(buf);
}

static BOSAudioStream s_audio_stream_inst __attribute__((aligned(16)));

BOSAudioStream* bos_audio_stream_create(void) {
    BOSAudioStream* s = &s_audio_stream_inst;
    memset(s, 0, sizeof(BOSAudioStream));

    display_print("[MEDIA-P3] AUDIO_STREAM_CREATE\n");
    display_print("[MEDIA-P3] AUDIO_BUFFER_ALLOC: size=");
    audio_print_num(BOS_AUDIO_FIFO_CAPACITY);
    display_print(" bytes\n");

    return s;
}

int bos_audio_stream_configure(BOSAudioStream* s, uint32_t sample_rate, uint8_t channels, uint8_t bit_depth) {
    if (!s) return -1;

    // Negotiate format with backend
    uint32_t negotiated_rate = sample_rate;
    if (negotiated_rate != 44100 && negotiated_rate != 48000 && negotiated_rate != 22050 && negotiated_rate != 32000) {
        negotiated_rate = 44100; // Standard fallback
    }

    uint8_t negotiated_channels = (channels == 1 || channels == 2) ? channels : 2;
    uint8_t negotiated_bits = (bit_depth == 16) ? 16 : 16;

    s->format.format = PCM_FORMAT_S16_LE;
    s->format.sample_rate = negotiated_rate;
    s->format.channels = negotiated_channels;
    s->format.bit_depth = negotiated_bits;
    s->format.is_signed = true;

    display_print("[MEDIA-P3] AUDIO_FORMAT_NEGOTIATE: rate=");
    audio_print_num(s->format.sample_rate);
    display_print(" ch=");
    audio_print_num((uint64_t)s->format.channels);
    display_print(" bits=");
    audio_print_num((uint64_t)s->format.bit_depth);
    display_print("\n");

    // Allocate kernel-side Audio HAL stream
    s->kernel_stream_id = audio_stream_create(0);
    if (s->kernel_stream_id == 0) {
        display_print("[MEDIA-P3] AUDIO_STREAM_CREATE: Failed in Audio HAL\n");
        return -1;
    }

    audio_stream_set_format(s->kernel_stream_id, &s->format);
    audio_set_volume(s->kernel_stream_id, 255);
    s->is_configured = true;

    return 0;
}

int bos_audio_stream_start(BOSAudioStream* s) {
    if (!s || !s->is_configured || s->kernel_stream_id == 0) return -1;
    audio_stream_resume(s->kernel_stream_id);
    s->is_playing = true;
    display_print("[MEDIA-P3] AUDIO_STREAM_START\n");
    return 0;
}

static int flush_fifo_chunk_to_kernel(BOSAudioStream* s, size_t max_bytes) {
    if (!s || s->fifo_queued_bytes == 0 || s->kernel_stream_id == 0) return 0;

    size_t to_flush = (max_bytes < s->fifo_queued_bytes) ? max_bytes : s->fifo_queued_bytes;
    if (to_flush > 8192) to_flush = 8192; // Chunk size for kernel submission

    // Ensure contiguous chunk
    size_t contig = BOS_AUDIO_FIFO_CAPACITY - s->fifo_read_pos;
    if (to_flush > contig) to_flush = contig;

    AudioPcmPacket pkt;
    pkt.format = s->format;
    size_t bpf = (s->format.channels * (s->format.bit_depth / 8));
    if (bpf == 0) bpf = 4;
    pkt.frame_count = (uint32_t)(to_flush / bpf);
    pkt.timestamp = 0;
    pkt.flags = 0;
    pkt.pcm_data = s->fifo_buffer + s->fifo_read_pos;
    pkt.size_bytes = to_flush;

    size_t written = audio_stream_write(s->kernel_stream_id, &pkt);
    if (written > 0) {
        s->fifo_read_pos = (s->fifo_read_pos + written) % BOS_AUDIO_FIFO_CAPACITY;
        s->fifo_queued_bytes -= written;
        s->stat_total_bytes_written += written;
        s->stat_total_frames += (written / bpf);

        display_print("[MEDIA-P3] AUDIO_BUFFER_CONSUME: bytes=");
        audio_print_num(written);
        display_print("\n");
        display_print("[MEDIA-P3] AUDIO_BUFFER_RELEASE\n");
        return (int)written;
    }
    return 0;
}

int bos_audio_stream_write(BOSAudioStream* s, const void* pcm_data, size_t size_bytes) {
    if (!s || !pcm_data || size_bytes == 0) return 0;

    // Check bounded FIFO capacity
    size_t avail = BOS_AUDIO_FIFO_CAPACITY - s->fifo_queued_bytes;
    if (size_bytes > avail) {
        s->stat_backpressures++;
        display_print("[MEDIA-P3] AUDIO_BACKPRESSURE\n");
        // Drain pending queue to kernel to create headroom
        flush_fifo_chunk_to_kernel(s, size_bytes);
        avail = BOS_AUDIO_FIFO_CAPACITY - s->fifo_queued_bytes;
        if (size_bytes > avail) {
            s->stat_overruns++;
            display_print("[MEDIA-P3] AUDIO_OVERRUN\n");
            size_bytes = avail; // Drop tail on overflow
        }
    }

    if (size_bytes == 0) return 0;

    // Copy incoming PCM data into circular FIFO
    const uint8_t* in = (const uint8_t*)pcm_data;
    size_t part1 = BOS_AUDIO_FIFO_CAPACITY - s->fifo_write_pos;
    if (size_bytes <= part1) {
        memcpy(s->fifo_buffer + s->fifo_write_pos, in, size_bytes);
        s->fifo_write_pos = (s->fifo_write_pos + size_bytes) % BOS_AUDIO_FIFO_CAPACITY;
    } else {
        memcpy(s->fifo_buffer + s->fifo_write_pos, in, part1);
        memcpy(s->fifo_buffer, in + part1, size_bytes - part1);
        s->fifo_write_pos = size_bytes - part1;
    }

    s->fifo_queued_bytes += size_bytes;

    display_print("[MEDIA-P3] AUDIO_BUFFER_QUEUE: bytes=");
    audio_print_num(size_bytes);
    display_print("\n");
    display_print("[MEDIA-P3] AUDIO_QUEUE_DEPTH: depth=");
    audio_print_num(s->fifo_queued_bytes);
    display_print("\n");

    // Flush a chunk to the kernel mixer if running
    flush_fifo_chunk_to_kernel(s, 8192);

    return (int)size_bytes;
}

int bos_audio_stream_write_fltp(BOSAudioStream* s, const float* const* channels, size_t num_samples, int num_channels) {
    if (!s || !channels || num_samples == 0 || num_channels <= 0) return 0;

    static int16_t s_interleaved_scratch[4096];
    size_t batch = num_samples;
    if (batch > 2048) batch = 2048;

    if (num_channels == 2 && channels[0] && channels[1]) {
        for (size_t i = 0; i < batch; i++) {
            float l = channels[0][i];
            float r = channels[1][i];
            if (l > 1.0f) l = 1.0f; else if (l < -1.0f) l = -1.0f;
            if (r > 1.0f) r = 1.0f; else if (r < -1.0f) r = -1.0f;
            s_interleaved_scratch[i * 2]     = (int16_t)(l * 32767.0f);
            s_interleaved_scratch[i * 2 + 1] = (int16_t)(r * 32767.0f);
        }
        return bos_audio_stream_write(s, s_interleaved_scratch, batch * 2 * sizeof(int16_t));
    } else if (num_channels == 1 && channels[0]) {
        // Mono to stereo duplicate
        for (size_t i = 0; i < batch; i++) {
            float m = channels[0][i];
            if (m > 1.0f) m = 1.0f; else if (m < -1.0f) m = -1.0f;
            int16_t s16 = (int16_t)(m * 32767.0f);
            s_interleaved_scratch[i * 2]     = s16;
            s_interleaved_scratch[i * 2 + 1] = s16;
        }
        return bos_audio_stream_write(s, s_interleaved_scratch, batch * 2 * sizeof(int16_t));
    }

    return 0;
}

int bos_audio_stream_pause(BOSAudioStream* s) {
    if (!s || s->kernel_stream_id == 0) return -1;
    audio_stream_pause(s->kernel_stream_id);
    s->is_playing = false;
    return 0;
}

int bos_audio_stream_resume(BOSAudioStream* s) {
    if (!s || s->kernel_stream_id == 0) return -1;
    audio_stream_resume(s->kernel_stream_id);
    s->is_playing = true;
    return 0;
}

int bos_audio_stream_drain(BOSAudioStream* s) {
    if (!s) return 0;
    display_print("[MEDIA-P3] AUDIO_STREAM_DRAIN\n");
    while (s->fifo_queued_bytes > 0) {
        if (flush_fifo_chunk_to_kernel(s, 8192) == 0) break;
    }
    return 0;
}

int bos_audio_stream_flush(BOSAudioStream* s) {
    if (!s) return 0;
    s->fifo_write_pos = 0;
    s->fifo_read_pos = 0;
    s->fifo_queued_bytes = 0;
    if (s->kernel_stream_id != 0) {
        audio_stream_flush(s->kernel_stream_id);
    }
    return 0;
}

int bos_audio_stream_stop(BOSAudioStream* s) {
    if (!s) return 0;
    if (s->kernel_stream_id != 0) {
        audio_stream_stop(s->kernel_stream_id);
    }
    s->is_playing = false;
    display_print("[MEDIA-P3] AUDIO_STREAM_STOP\n");
    return 0;
}

void bos_audio_stream_destroy(BOSAudioStream* s) {
    if (!s) return;
    if (s->kernel_stream_id != 0) {
        audio_stream_stop(s->kernel_stream_id);
        audio_stream_destroy(s->kernel_stream_id);
        s->kernel_stream_id = 0;
    }
    s->is_configured = false;
    s->is_playing = false;
    s->fifo_queued_bytes = 0;
    display_print("[MEDIA-P3] AUDIO_STREAM_DESTROY\n");
}

size_t bos_audio_stream_get_queued_bytes(BOSAudioStream* s) {
    return s ? s->fifo_queued_bytes : 0;
}

size_t bos_audio_stream_get_available_space(BOSAudioStream* s) {
    return s ? (BOS_AUDIO_FIFO_CAPACITY - s->fifo_queued_bytes) : 0;
}
