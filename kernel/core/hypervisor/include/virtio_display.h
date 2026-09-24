/*
 * ATOMS OS — VirtIO Display & Shared Framebuffer Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3 & Phase 5A-4: VirtIO Virtual Hardware Subsystem & Real Graphics Pipeline
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

#define VIRTIO_GPU_QUEUE_CTRL               0
#define VIRTIO_GPU_QUEUE_CURSOR             1

/* VirtIO GPU 2D Protocol Command Opcodes (VirtIO Spec 5.7) */
#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO        0x0100
#define VIRTIO_GPU_CMD_RESOURCE_CREATE_2D      0x0101
#define VIRTIO_GPU_CMD_RESOURCE_UNREF          0x0102
#define VIRTIO_GPU_CMD_SET_SCANOUT             0x0103
#define VIRTIO_GPU_CMD_RESOURCE_FLUSH          0x0104
#define VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D     0x0105
#define VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING 0x0106
#define VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING 0x0107
#define VIRTIO_GPU_CMD_GET_CAPSET_INFO         0x0108
#define VIRTIO_GPU_CMD_GET_CAPSET              0x0109
#define VIRTIO_GPU_CMD_GET_EDID                0x010A

/* VirtIO GPU Response Opcodes */
#define VIRTIO_GPU_RESP_OK_NODATA              0x1100
#define VIRTIO_GPU_RESP_OK_DISPLAY_INFO        0x1101
#define VIRTIO_GPU_RESP_OK_CAPSET_INFO         0x1102
#define VIRTIO_GPU_RESP_OK_CAPSET              0x1103
#define VIRTIO_GPU_RESP_OK_EDID                0x1104
#define VIRTIO_GPU_RESP_ERR_UNSPEC             0x1200
#define VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY      0x1201
#define VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID 0x1202
#define VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID 0x1203
#define VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID 0x1204
#define VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER  0x1205

/* VirtIO GPU Formats */
#define VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM       1
#define VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM       2
#define VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM       3
#define VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM       4
#define VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM       67
#define VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM       68

#define VIRTIO_GPU_MAX_SCANOUTS                16
#define VIRTIO_GPU_MAX_RESOURCES               16

/* VirtIO GPU Protocol Header */
typedef struct virtio_gpu_ctrl_hdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_ctrl_hdr_t;

typedef struct virtio_gpu_rect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_rect_t;

typedef struct virtio_gpu_resp_display_info {
    virtio_gpu_ctrl_hdr_t hdr;
    struct {
        virtio_gpu_rect_t r;
        uint32_t enabled;
        uint32_t flags;
    } pmodes[VIRTIO_GPU_MAX_SCANOUTS];
} __attribute__((packed)) virtio_gpu_resp_display_info_t;

typedef struct virtio_gpu_resource_create_2d {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} __attribute__((packed)) virtio_gpu_resource_create_2d_t;

typedef struct virtio_gpu_resource_attach_backing {
    virtio_gpu_ctrl_hdr_t hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
} __attribute__((packed)) virtio_gpu_resource_attach_backing_t;

typedef struct virtio_gpu_mem_entry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_mem_entry_t;

typedef struct virtio_gpu_set_scanout {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t scanout_id;
    uint32_t resource_id;
} __attribute__((packed)) virtio_gpu_set_scanout_t;

typedef struct virtio_gpu_transfer_to_host_2d {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint64_t offset;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_transfer_to_host_2d_t;

typedef struct virtio_gpu_resource_flush {
    virtio_gpu_ctrl_hdr_t hdr;
    virtio_gpu_rect_t r;
    uint32_t resource_id;
    uint32_t padding;
} __attribute__((packed)) virtio_gpu_resource_flush_t;

/* Internal 2D Resource Descriptor */
typedef struct virtio_gpu_resource {
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint64_t backing_gpa;
    uint32_t backing_len;
    bool in_use;
} virtio_gpu_resource_t;

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

    /* Dirty Region Tracking */
    uint32_t dirty_x;
    uint32_t dirty_y;
    uint32_t dirty_w;
    uint32_t dirty_h;
    bool has_dirty_rect;

    /* Resources */
    virtio_gpu_resource_t resources[VIRTIO_GPU_MAX_RESOURCES];
    uint32_t active_scanout_resource_id;

    /* Metrics & Statistics */
    uint64_t total_commands;
    uint64_t total_flushes;
    uint64_t total_frames_presented;
    bool driver_active;
    bool guest_owns_display;
} VirtIODisplay;

/* Global Active Display Pointer */
extern VirtIODisplay *g_active_virtio_display;

/* Core APIs */
VirtIODisplay *virtio_display_create(uint32_t width, uint32_t height);
void virtio_display_destroy(VirtIODisplay *disp);

/* Framebuffer Configuration & Flush */
bool virtio_display_set_scanout(VirtIODisplay *disp, uint64_t gpa_base, uint32_t width, uint32_t height, uint32_t pitch);
void virtio_display_flush(VirtIODisplay *disp, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void virtio_display_blit_to_host(VirtIODisplay *disp, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_DISPLAY_H */
