/*
 * ATOMS OS — VirtIO Display & Shared Framebuffer Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_DISPLAY_H
#define ATOMS_VIRTIO_DISPLAY_H

#include "virtio_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTIO_GPU_DEFAULT_WIDTH            1024
#define VIRTIO_GPU_DEFAULT_HEIGHT           768
#define VIRTIO_GPU_DEFAULT_BPP              32

/* VirtIO Display / Framebuffer Device Instance */
typedef struct virtio_display_dev {
    VirtIODevice *base;

    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;

    /* Shared Linear Framebuffer Memory */
    uint64_t gpa_framebuffer;
    void *hva_framebuffer;
    uint32_t framebuffer_size;

    /* Dirty Region Tracking for BCM Compositor */
    uint32_t dirty_x;
    uint32_t dirty_y;
    uint32_t dirty_w;
    uint32_t dirty_h;
    bool has_dirty_rect;

    /* Metrics & Statistics */
    uint64_t total_flushes;
    uint64_t total_frames_presented;
} VirtIODisplay;

/* Core APIs */
VirtIODisplay *virtio_display_create(uint32_t width, uint32_t height);
void virtio_display_destroy(VirtIODisplay *disp);

/* Framebuffer Configuration & Flush */
bool virtio_display_set_scanout(VirtIODisplay *disp, uint64_t gpa_base, uint32_t width, uint32_t height, uint32_t pitch);
void virtio_display_flush(VirtIODisplay *disp, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_DISPLAY_H */
