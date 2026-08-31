/*
 * ATOMS OS — Userspace Audio API Platform Adapter
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/api/audio_api.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static uint32_t s_next_audio_stream = 1;
static uint8_t s_global_volume = 100;

void audio_init(void) {}
void audio_shutdown(void) {}

uint32_t audio_stream_create(uint32_t process_id) {
    (void)process_id;
    return s_next_audio_stream++;
}

bool audio_stream_destroy(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

bool audio_stream_pause(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

bool audio_stream_resume(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

bool audio_stream_stop(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

bool audio_stream_set_format(uint32_t stream_id, const AudioPcmFormat* format) {
    (void)stream_id; (void)format;
    return true;
}

size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {
    (void)stream_id;
    if (!packet) return 0;
    return packet->frame_count;
}

size_t audio_stream_read(uint32_t stream_id, uint8_t* buffer, size_t size_bytes) {
    (void)stream_id; (void)buffer; (void)size_bytes;
    return 0;
}

bool audio_stream_flush(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

bool audio_stream_reset(uint32_t stream_id) {
    (void)stream_id;
    return true;
}

size_t audio_stream_available(uint32_t stream_id) {
    (void)stream_id;
    return 4096;
}

size_t audio_stream_capacity(uint32_t stream_id) {
    (void)stream_id;
    return 65536;
}

bool audio_set_volume(uint32_t stream_id, uint8_t volume) {
    (void)stream_id;
    s_global_volume = volume;
    return true;
}

void audio_get_stats(void) {}
