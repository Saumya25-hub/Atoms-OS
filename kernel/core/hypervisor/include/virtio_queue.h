/*
 * ATOMS OS — VirtIO Queue Abstraction & Descriptor Processing
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_QUEUE_H
#define ATOMS_VIRTIO_QUEUE_H

#include "virtio_types.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRTQUEUE_MAX_SIZE                  1024
#define VIRTQUEUE_DEFAULT_ALIGN             4096
#define VIRTQUEUE_MAX_CHAIN_DESCRIPTORS     128

/* Deserialized Single Descriptor Element with validated Host Virtual Address (HVA) */
typedef struct {
    uint64_t gpa;
    void *hva;
    uint32_t len;
    uint16_t flags;
    bool is_write;  /* Buffer is writable by host device */
} VirtQueueBuffer;

/* Validated In-Flight Descriptor Chain */
typedef struct {
    uint32_t head_index;
    uint32_t count;
    VirtQueueBuffer buffers[VIRTQUEUE_MAX_CHAIN_DESCRIPTORS];
    uint32_t total_readable_len;
    uint32_t total_writable_len;
} VirtQueueChain;

/* VirtQueue State */
typedef struct virtqueue {
    uint32_t queue_index;
    uint16_t queue_size;
    uint16_t queue_align;
    bool ready;

    /* Guest Physical Addresses (Configured by Guest OS Driver) */
    uint64_t desc_gpa;
    uint64_t avail_gpa;
    uint64_t used_gpa;

    /* Legacy PFN Configuration (for VirtIO 0.9.5 Legacy Mode) */
    uint32_t pfn;

    /* Host Shadow Ring Indexes */
    uint16_t last_avail_idx;
    uint16_t last_used_idx;

    /* Device Callback for Event Injection */
    void (*notify_host_cb)(struct virtqueue *vq, void *opaque);
    void *opaque;
} VirtQueue;

/* VirtQueue Core Management APIs */
VirtQueue *virtio_queue_create(uint32_t index, uint16_t size, uint16_t align);
void virtio_queue_destroy(VirtQueue *vq);
void virtio_queue_reset(VirtQueue *vq);

/* Configuration Interface */
bool virtio_queue_configure(VirtQueue *vq, uint64_t desc_gpa, uint64_t avail_gpa, uint64_t used_gpa);
bool virtio_queue_set_pfn(VirtQueue *vq, uint32_t pfn, uint32_t page_size);

/* Descriptor Processing & Completion */
bool virtio_queue_has_available(VirtQueue *vq, GuestMemory *mem);
bool virtio_queue_pop_chain(VirtQueue *vq, GuestMemory *mem, VirtQueueChain *out_chain);
bool virtio_queue_complete_chain(VirtQueue *vq, GuestMemory *mem, uint32_t head_index, uint32_t written_len);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_QUEUE_H */
