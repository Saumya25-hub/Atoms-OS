/*
 * ============================================================================
 * ATOMS OS — Native Media Engine Diagnostics & Telemetry
 * userspace/libbos_media/src/bos_media_telemetry.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements structured [MEDIA], [MEDIA_SYNC], and [MEDIA_PERF] telemetry.
 * Zero fake metrics, zero synthetic frame CRC generation.
 * ============================================================================
 */

#include "../include/bos_media.h"
#include "userspace/runtime/c/include/atoms_syscall.h"
#include <string.h>

static void print_str(const char* s) {
    if (!s) return;
    size_t len = 0;
    while (s[len]) len++;
    if (len > 0) {
        __atoms_syscall2(SYS_WRITE, (uint64_t)s, (uint64_t)len);
    }
}

static void print_u32(uint32_t val) {
    char buf[16], rev[16];
    int r = 0;
    if (val == 0) { print_str("0"); return; }
    while (val > 0) { rev[r++] = '0' + (val % 10); val /= 10; }
    for (int i = 0; i < r; i++) buf[i] = rev[r - 1 - i];
    buf[r] = '\0';
    print_str(buf);
}

static void print_i64(int64_t val) {
    if (val < 0) {
        print_str("-");
        val = -val;
    }
    char buf[32], rev[32];
    int r = 0;
    if (val == 0) { print_str("0"); return; }
    while (val > 0) { rev[r++] = '0' + (val % 10); val /= 10; }
    for (int i = 0; i < r; i++) buf[i] = rev[r - 1 - i];
    buf[r] = '\0';
    print_str(buf);
}

extern "C" void bos_media_report_stage(const char* stage, const char* status, const char* details) {
    print_str("[MEDIA] ");
    print_str(stage);
    print_str(": ");
    print_str(status);
    if (details && details[0]) {
        print_str(" (");
        print_str(details);
        print_str(")");
    }
    print_str("\n");
}

extern "C" void bos_media_dump_sync_telemetry(BOSMediaPlayer* player) {
    if (!player) return;
    BOSMediaTelemetry tel;
    if (bos_media_get_telemetry(player, &tel) != BOS_MEDIA_OK) return;

    print_str("[MEDIA_SYNC] AUDIO_PTS=");
    print_i64(tel.audio_pts_ms);
    print_str(" VIDEO_PTS=");
    print_i64(tel.video_pts_ms);
    print_str(" DRIFT_MS=");
    print_i64(tel.drift_ms);
    print_str(" DROPPED_FRAMES=");
    print_u32(tel.dropped_frames);
    print_str(" PRESENTED=");
    print_u32(tel.presented_frames);
    print_str("\n");
}

extern "C" void bos_media_dump_perf_telemetry(BOSMediaPlayer* player, uint64_t startup_ms, uint64_t first_frame_ms, double fps) {
    if (!player) return;
    BOSMediaTelemetry tel;
    if (bos_media_get_telemetry(player, &tel) != BOS_MEDIA_OK) return;

    print_str("[MEDIA_PERF] STARTUP_MS=");
    print_u32((uint32_t)startup_ms);
    print_str(" FIRST_FRAME_MS=");
    print_u32((uint32_t)first_frame_ms);
    print_str(" FPS=");
    print_u32((uint32_t)fps);
    print_str(" HW_ACCEL=");
    print_str(tel.is_hardware_accelerated ? "YES" : "NO");
    print_str(" BACKEND=");
    print_str(tel.acceleration_backend ? tel.acceleration_backend : "SOFTWARE");
    print_str("\n");
}
