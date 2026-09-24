/*
 * ATOMS OS — VirtIO Display & Shared Framebuffer Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3 & Phase 5A-4: VirtIO Virtual Hardware Subsystem & Real Graphics Pipeline
 */

#include "kernel/core/hypervisor/include/virtio_display.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/abde/abde.h"

extern void com1_puts(const char *s);

/* Active VirtIO-GPU Singleton */
VirtIODisplay *g_active_virtio_display = NULL;

static void display_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    if (!dev || !dev->vm || !dev->vm->guest_mem) return;
    VirtIODisplay *disp = (VirtIODisplay *)dev->backend_data;
    if (!disp) return;

    if (q_idx != VIRTIO_GPU_QUEUE_CTRL) return;

    VirtQueue *vq = dev->queues[VIRTIO_GPU_QUEUE_CTRL];
    GuestMemory *mem = dev->vm->guest_mem;

    VirtQueueChain chain;
    while (virtio_queue_pop_chain(vq, mem, &chain)) {
        if (chain.count < 1) continue;

        disp->total_commands++;
        disp->driver_active = true;

        VirtQueueBuffer *req_buf = NULL;
        VirtQueueBuffer *resp_buf = NULL;

        for (uint32_t i = 0; i < chain.count; i++) {
            if (!chain.buffers[i].is_write && !req_buf) {
                req_buf = &chain.buffers[i];
            } else if (chain.buffers[i].is_write && !resp_buf) {
                resp_buf = &chain.buffers[i];
            }
        }

        if (!req_buf || !req_buf->hva || req_buf->len < sizeof(virtio_gpu_ctrl_hdr_t)) {
            virtio_queue_complete_chain(vq, mem, chain.head_index, 0);
            continue;
        }

        virtio_gpu_ctrl_hdr_t *cmd_hdr = (virtio_gpu_ctrl_hdr_t *)req_buf->hva;
        uint32_t resp_len = 0;

        if (resp_buf && resp_buf->hva && resp_buf->len >= sizeof(virtio_gpu_ctrl_hdr_t)) {
            virtio_gpu_ctrl_hdr_t *resp_hdr = (virtio_gpu_ctrl_hdr_t *)resp_buf->hva;
            memset(resp_buf->hva, 0, resp_buf->len);

            switch (cmd_hdr->type) {
                case VIRTIO_GPU_CMD_GET_DISPLAY_INFO: {
                    if (resp_buf->len >= sizeof(virtio_gpu_resp_display_info_t)) {
                        virtio_gpu_resp_display_info_t *dinfo = (virtio_gpu_resp_display_info_t *)resp_buf->hva;
                        dinfo->hdr.type = VIRTIO_GPU_RESP_OK_DISPLAY_INFO;
                        dinfo->pmodes[0].enabled = 1;
                        dinfo->pmodes[0].flags = 0;
                        dinfo->pmodes[0].r.x = 0;
                        dinfo->pmodes[0].r.y = 0;
                        dinfo->pmodes[0].r.width = disp->width;
                        dinfo->pmodes[0].r.height = disp->height;
                        resp_len = sizeof(virtio_gpu_resp_display_info_t);
                    } else {
                        resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                        resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    }
                    break;
                }

                case VIRTIO_GPU_CMD_RESOURCE_CREATE_2D: {
                    virtio_gpu_resource_create_2d_t *c2d = (virtio_gpu_resource_create_2d_t *)cmd_hdr;
                    uint32_t rid = c2d->resource_id;

                    /* Allocate or update resource slot */
                    int slot = -1;
                    for (int r = 0; r < VIRTIO_GPU_MAX_RESOURCES; r++) {
                        if (disp->resources[r].in_use && disp->resources[r].resource_id == rid) {
                            slot = r;
                            break;
                        }
                        if (!disp->resources[r].in_use && slot == -1) {
                            slot = r;
                        }
                    }

                    if (slot >= 0) {
                        disp->resources[slot].resource_id = rid;
                        disp->resources[slot].format = c2d->format;
                        disp->resources[slot].width = c2d->width;
                        disp->resources[slot].height = c2d->height;
                        disp->resources[slot].backing_gpa = 0;
                        disp->resources[slot].backing_len = 0;
                        disp->resources[slot].in_use = true;
                        resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    } else {
                        resp_hdr->type = VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY;
                    }
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                case VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING: {
                    virtio_gpu_resource_attach_backing_t *att = (virtio_gpu_resource_attach_backing_t *)cmd_hdr;
                    uint32_t rid = att->resource_id;

                    if (req_buf->len >= sizeof(virtio_gpu_resource_attach_backing_t) + sizeof(virtio_gpu_mem_entry_t)) {
                        virtio_gpu_mem_entry_t *entry = (virtio_gpu_mem_entry_t *)((uint8_t *)cmd_hdr + sizeof(virtio_gpu_resource_attach_backing_t));
                        for (int r = 0; r < VIRTIO_GPU_MAX_RESOURCES; r++) {
                            if (disp->resources[r].in_use && disp->resources[r].resource_id == rid) {
                                disp->resources[r].backing_gpa = entry->addr;
                                disp->resources[r].backing_len = entry->length;
                                break;
                            }
                        }
                    }
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                case VIRTIO_GPU_CMD_SET_SCANOUT: {
                    virtio_gpu_set_scanout_t *scanout = (virtio_gpu_set_scanout_t *)cmd_hdr;
                    uint32_t rid = scanout->resource_id;

                    for (int r = 0; r < VIRTIO_GPU_MAX_RESOURCES; r++) {
                        if (disp->resources[r].in_use && disp->resources[r].resource_id == rid) {
                            virtio_gpu_resource_t *res = &disp->resources[r];
                            if (res->backing_gpa < mem->gpa_size) {
                                disp->gpa_framebuffer = res->backing_gpa;
                                disp->hva_framebuffer = (void *)((uintptr_t)mem->hva_backing + (uintptr_t)res->backing_gpa);
                                disp->width = scanout->r.width ? scanout->r.width : res->width;
                                disp->height = scanout->r.height ? scanout->r.height : res->height;
                                disp->pitch = disp->width * 4;
                                disp->framebuffer_size = disp->pitch * disp->height;
                                disp->active_scanout_resource_id = rid;
                            }
                            break;
                        }
                    }
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                case VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D: {
                    virtio_gpu_transfer_to_host_2d_t *xfer = (virtio_gpu_transfer_to_host_2d_t *)cmd_hdr;
                    disp->dirty_x = xfer->r.x;
                    disp->dirty_y = xfer->r.y;
                    disp->dirty_w = xfer->r.width;
                    disp->dirty_h = xfer->r.height;
                    disp->has_dirty_rect = true;
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                case VIRTIO_GPU_CMD_RESOURCE_FLUSH: {
                    virtio_gpu_resource_flush_t *fl = (virtio_gpu_resource_flush_t *)cmd_hdr;
                    disp->total_flushes++;
                    disp->total_frames_presented++;
                    virtio_display_blit_to_host(disp, fl->r.x, fl->r.y, fl->r.width, fl->r.height);
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                case VIRTIO_GPU_CMD_RESOURCE_UNREF: {
                    virtio_gpu_ctrl_hdr_t *unref = (virtio_gpu_ctrl_hdr_t *)cmd_hdr;
                    uint32_t rid = ((uint32_t *)unref)[6]; /* resource_id after hdr */
                    for (int r = 0; r < VIRTIO_GPU_MAX_RESOURCES; r++) {
                        if (disp->resources[r].in_use && disp->resources[r].resource_id == rid) {
                            disp->resources[r].in_use = false;
                            break;
                        }
                    }
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
                }

                default:
                    resp_hdr->type = VIRTIO_GPU_RESP_OK_NODATA;
                    resp_len = sizeof(virtio_gpu_ctrl_hdr_t);
                    break;
            }
        }

        virtio_queue_complete_chain(vq, mem, chain.head_index, resp_len);
        virtio_device_raise_interrupt(dev, 0x01);
    }
}

static void display_reset_cb(VirtIODevice *dev) {
    if (!dev) return;
    VirtIODisplay *disp = (VirtIODisplay *)dev->backend_data;
    if (disp) {
        disp->has_dirty_rect = false;
        disp->driver_active = false;
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
    disp->guest_owns_display = false;
    disp->driver_active = false;

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

    g_active_virtio_display = disp;
    return disp;
}

void virtio_display_destroy(VirtIODisplay *disp) {
    if (!disp) return;

    if (g_active_virtio_display == disp) {
        g_active_virtio_display = NULL;
    }

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
    disp->driver_active = true;

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

    virtio_display_blit_to_host(disp, x, y, w, h);
}

void virtio_display_blit_to_host(VirtIODisplay *disp, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!disp || !disp->hva_framebuffer || !disp->guest_owns_display) return;
    extern abde_engine_t g_abde;
    if (!g_abde.framebuffer) return;

    uint32_t screen_w = g_abde.width;
    uint32_t screen_h = g_abde.height;
    uint32_t screen_pitch = g_abde.pitch;
    uint8_t *host_fb = (uint8_t *)(uintptr_t)g_abde.framebuffer;
    uint8_t *guest_fb = (uint8_t *)disp->hva_framebuffer;

    if (x >= screen_w || y >= screen_h) return;
    if (x + w > screen_w) w = screen_w - x;
    if (y + h > screen_h) h = screen_h - y;
    if (w == 0 || h == 0) return;

    for (uint32_t row = 0; row < h; row++) {
        uint32_t gy = y + row;
        if (gy >= disp->height) break;
        uint32_t guest_off = gy * disp->pitch + (x * 4);
        uint32_t host_off  = gy * screen_pitch + (x * 4);
        memcpy(host_fb + host_off, guest_fb + guest_off, w * 4);
    }
}
