/*
 * ============================================================================
 * ATOMS OS — BOS libmpv Video Presentation Output Bridge
 * userspace/libbos_media/mpv/mpv_video_output.cpp
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Renders decoded video frames via mpv_render_context (SW/HW blit)
 * directly into BOSurface ARGB32 framebuffers with aspect-ratio scaling.
 * ============================================================================
 */

#include "mpv_adapter.h"
#include <string.h>

// Simple fast CRC32 for frame proof verification (Section 48 requirement)
static uint32_t compute_frame_crc32(const uint32_t* pixels, size_t count) {
    if (!pixels || count == 0) return 0;
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* p = (const uint8_t*)pixels;
    size_t bytes = count * sizeof(uint32_t);
    // Sample step to keep telemetry lightning-fast on 1080p
    size_t step = (bytes > 65536) ? 32 : 4;
    for (size_t i = 0; i < bytes; i += step) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

int mpv_adapter_set_video_surface(BOSMpvAdapter* adapter, void* surface_handle) {
    if (!adapter) return BOS_MEDIA_ERROR_INVALID_PARAM;
    adapter->surface_handle = surface_handle;
    return BOS_MEDIA_OK;
}

int mpv_adapter_render_frame(BOSMpvAdapter* adapter, uint32_t* target_fb, int target_w, int target_h, int stride_pixels) {
    if (!adapter || !target_fb || target_w <= 0 || target_h <= 0) {
        return BOS_MEDIA_ERROR_INVALID_PARAM;
    }

    if (!adapter->meta.has_video) {
        // Audio-only media: fill viewport with audio visualizer background
        for (int y = 0; y < target_h; y++) {
            uint32_t* row = target_fb + y * stride_pixels;
            for (int x = 0; x < target_w; x++) {
                row[x] = 0xFF111827; // Deep charcoal
            }
        }
        return BOS_MEDIA_OK;
    }

    // Direct Software Blit via mpv_render_context
    if (adapter->render_ctx) {
        int size[2] = { target_w, target_h };
        size_t stride_bytes = (size_t)(stride_pixels * sizeof(uint32_t));
        const char* fmt = "bgr0"; // 32bpp linear ARGB/XRGB

        mpv_render_param params[] = {
            { MPV_RENDER_PARAM_SW_SIZE,    size },
            { MPV_RENDER_PARAM_SW_FORMAT,  (void*)fmt },
            { MPV_RENDER_PARAM_SW_STRIDE,  &stride_bytes },
            { MPV_RENDER_PARAM_SW_POINTER, target_fb },
            { MPV_RENDER_PARAM_INVALID,    nullptr }
        };

        int err = mpv_render_context_render(adapter->render_ctx, params);
        if (err == 0) {
            adapter->telemetry.presented_frames++;
            adapter->telemetry.video_pts_ms = adapter->last_frame_pts;
            adapter->frame_crc = compute_frame_crc32(target_fb, target_w * target_h);
            return BOS_MEDIA_OK;
        }
    }

    // Direct buffer copy if internal frame buffer is present
    if (adapter->video_frame_buffer && adapter->video_width > 0 && adapter->video_height > 0) {
        // Calculate aspect-ratio preserving viewport (letterboxing/pillarboxing)
        int src_w = adapter->video_width;
        int src_h = adapter->video_height;
        int dst_w = target_w;
        int dst_h = (dst_w * src_h) / src_w;

        if (dst_h > target_h) {
            dst_h = target_h;
            dst_w = (dst_h * src_w) / src_h;
        }

        int start_x = (target_w - dst_w) / 2;
        int start_y = (target_h - dst_h) / 2;

        // Fill background bars with deep cinematic black
        for (int y = 0; y < target_h; y++) {
            uint32_t* row = target_fb + y * stride_pixels;
            for (int x = 0; x < target_w; x++) {
                row[x] = 0xFF0A0F19;
            }
        }

        // Scaled blit from video_frame_buffer
        uint32_t step_x = ((uint32_t)src_w << 16) / dst_w;
        uint32_t step_y = ((uint32_t)src_h << 16) / dst_h;

        for (int dy = 0; dy < dst_h; dy++) {
            int py = start_y + dy;
            if (py < 0 || py >= target_h) continue;

            uint32_t sy = (dy * step_y) >> 16;
            if (sy >= (uint32_t)src_h) sy = src_h - 1;

            const uint32_t* src_row = adapter->video_frame_buffer + sy * src_w;
            uint32_t* dst_row = target_fb + py * stride_pixels + start_x;

            for (int dx = 0; dx < dst_w; dx++) {
                uint32_t sx = (dx * step_x) >> 16;
                if (sx >= (uint32_t)src_w) sx = src_w - 1;
                dst_row[dx] = src_row[sx];
            }
        }

        adapter->telemetry.presented_frames++;
        adapter->telemetry.video_pts_ms = adapter->last_frame_pts;
        adapter->frame_crc = compute_frame_crc32(target_fb, target_w * target_h);
        return BOS_MEDIA_OK;
    }

    return BOS_MEDIA_ERROR_VIDEO_RENDER;
}
