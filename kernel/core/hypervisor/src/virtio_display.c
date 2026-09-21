/*
 * ATOMS OS — VirtIO Display & Shared Framebuffer Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_display.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static void display_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    (void)dev;
    (void)q_idx;
    /* 2D Control Queue processing hook */
}

static void display_reset_cb(VirtIODevice *dev) {
    if (!dev) return;
    VirtIODisplay *disp = (VirtIODisplay *)dev->backend_data;
    if (disp) {
        disp->has_dirty_rect = false;
    }
}

VirtIODisplay *virtio_display_create(uint32_t width, uint32_t height) {
    if (width == 0) width = VIRTIO_GPU_DEFAULT_WIDTH;
    if (height == 0) height = VIRTIO_GPU_DEFAULT_HEIGHT;

    VirtIODisplay *disp = (VirtIODisplay *)kmalloc(sizeof(VirtIODisplay));
    if (!disp) return NULL;
    memset(disp, 0, sizeof(VirtIODisplay));

    disp->width = width;
    disp->height = height;
    disp->bpp = VIRTIO_GPU_DEFAULT_BPP;
    disp->pitch = width * (disp->bpp / 8);
    disp->framebuffer_size = disp->pitch * height;

    /* Create underlying VirtIODevice (Device ID = 16 for GPU/Display, 2 Queues) */
    disp->base = virtio_device_create(VIRTIO_DEV_ID_GPU, VIRTIO_PCI_DEVICE_GPU, 2, 64);
    if (!disp->base) {
        kfree(disp);
        return NULL;
    }

    disp->base->backend_data = disp;
    disp->base->on_queue_notify = display_queue_notify_cb;
    disp->base->on_reset = display_reset_cb;
    disp->base->host_features = VIRTIO_F_VERSION_1;

    return disp;
}

void virtio_display_destroy(VirtIODisplay *disp) {
    if (!disp) return;

    if (disp->base) {
        virtio_device_destroy(disp->base);
        disp->base = NULL;
    }

    kfree(disp);
}

bool virtio_display_set_scanout(VirtIODisplay *disp, uint64_t gpa_base, uint32_t width, uint32_t height, uint32_t pitch) {
    if (!disp || !disp->base || !disp->base->vm || !disp->base->vm->guest_mem) return false;

    GuestMemory *mem = disp->base->vm->guest_mem;
    uint32_t needed_bytes = pitch * height;

    if (gpa_base + (uint64_t)needed_bytes > mem->gpa_size) {
        return false; /* Out of bounds GPA */
    }

    disp->gpa_framebuffer = gpa_base;
    disp->hva_framebuffer = (void *)((uintptr_t)mem->hva_backing + (uintptr_t)gpa_base);
    disp->width = width;
    disp->height = height;
    disp->pitch = pitch;
    disp->framebuffer_size = needed_bytes;

    return true;
}

void virtio_display_flush(VirtIODisplay *disp, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!disp) return;

    disp->dirty_x = x;
    disp->dirty_y = y;
    disp->dirty_w = w;
    disp->dirty_h = h;
    disp->has_dirty_rect = true;
    disp->total_flushes++;
    disp->total_frames_presented++;

    /* In a running desktop session, BCM compositor reads hva_framebuffer directly */
}
