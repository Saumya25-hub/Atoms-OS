/*
 * ATOMS OS — Userspace Audio API Platform Adapter
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/audio/api/audio_api.h"
#include "userspace/runtime/c/include/atoms_syscall.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void audio_init(void) {
    /* Probe audio hardware via kernel syscall */
    __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_DEVICE_GET_INFO, 0);
}

void audio_shutdown(void) {}

uint32_t audio_stream_create(uint32_t process_id) {
    (void)process_id;
    uint64_t ret = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_CREATE, 0);
    return (ret != (uint64_t)-1) ? (uint32_t)ret : 0;
}

bool audio_stream_destroy(uint32_t stream_id) {
    uint64_t ret = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_DESTROY, stream_id);
    return (ret == 0);
}

bool audio_stream_pause(uint32_t stream_id) {
    uint64_t ret = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_PAUSE, stream_id);
    return (ret == 0);
}

bool audio_stream_resume(uint32_t stream_id) {
    uint64_t ret = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_RESUME, stream_id);
    return (ret == 0);
}

bool audio_stream_stop(uint32_t stream_id) {
    uint64_t ret = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_STOP, stream_id);
    return (ret == 0);
}

bool audio_stream_set_format(uint32_t stream_id, const AudioPcmFormat* format) {
    if (!format) return false;
    uint64_t ret = __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_SET_FORMAT, stream_id, (uint64_t)format);
    return (ret == 0);
}

size_t audio_stream_write(uint32_t stream_id, const AudioPcmPacket* packet) {
    if (!packet) return 0;
    uint64_t written = __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_WRITE, stream_id, (uint64_t)packet);
    return (size_t)written;
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
    uint64_t avail = __atoms_syscall2(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_STREAM_GET_AVAIL, stream_id);
    return (size_t)avail;
}

size_t audio_stream_capacity(uint32_t stream_id) {
    (void)stream_id;
    return 65536;
}

bool audio_set_volume(uint32_t stream_id, uint8_t volume) {
    uint64_t ret = __atoms_syscall3(SYS_AUDIO_CALL, ATOMS_AUDIO_OP_DEVICE_SET_VOL, stream_id, volume);
    return (ret == 0);
}

void audio_get_stats(void) {}
