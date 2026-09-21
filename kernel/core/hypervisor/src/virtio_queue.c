/*
 * ATOMS OS — VirtIO Queue Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_queue.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

/* Helper: Align value up to power of two */
static inline uint64_t vring_align(uint64_t addr, uint64_t align) {
    return (addr + align - 1) & ~(align - 1);
}

VirtQueue *virtio_queue_create(uint32_t index, uint16_t size, uint16_t align) {
    if (size == 0 || size > VIRTQUEUE_MAX_SIZE || (size & (size - 1)) != 0) {
        return NULL; /* Size must be power of two */
    }

    VirtQueue *vq = (VirtQueue *)kmalloc(sizeof(VirtQueue));
    if (!vq) return NULL;
    memset(vq, 0, sizeof(VirtQueue));

    vq->queue_index = index;
    vq->queue_size = size;
    vq->queue_align = (align > 0) ? align : VIRTQUEUE_DEFAULT_ALIGN;
    vq->ready = false;
    vq->last_avail_idx = 0;
    vq->last_used_idx = 0;

    return vq;
}

void virtio_queue_destroy(VirtQueue *vq) {
    if (!vq) return;
    kfree(vq);
}

void virtio_queue_reset(VirtQueue *vq) {
    if (!vq) return;
    vq->ready = false;
    vq->desc_gpa = 0;
    vq->avail_gpa = 0;
    vq->used_gpa = 0;
    vq->pfn = 0;
    vq->last_avail_idx = 0;
    vq->last_used_idx = 0;
}

bool virtio_queue_configure(VirtQueue *vq, uint64_t desc_gpa, uint64_t avail_gpa, uint64_t used_gpa) {
    if (!vq) return false;

    if (desc_gpa == 0 || avail_gpa == 0 || used_gpa == 0) {
        vq->ready = false;
        return false;
    }

    vq->desc_gpa = desc_gpa;
    vq->avail_gpa = avail_gpa;
    vq->used_gpa = used_gpa;
    vq->last_avail_idx = 0;
    vq->last_used_idx = 0;
    vq->ready = true;

    return true;
}

bool virtio_queue_set_pfn(VirtQueue *vq, uint32_t pfn, uint32_t page_size) {
    if (!vq) return false;

    if (pfn == 0) {
        virtio_queue_reset(vq);
        return true;
    }

    if (page_size == 0) page_size = 4096;

    uint64_t base_gpa = (uint64_t)pfn * (uint64_t)page_size;
    uint16_t qsize = vq->queue_size;

    /* Legacy VirtIO Ring Layout:
     * 1. Descriptors: base_gpa (size = 16 * qsize)
     * 2. Available Ring: base_gpa + 16 * qsize (size = 4 + 2 * qsize)
     * 3. Used Ring: ALIGNED(AvailEnd, align) (size = 4 + 8 * qsize)
     */
    uint64_t desc_gpa = base_gpa;
    uint64_t avail_gpa = desc_gpa + (16ULL * qsize);
    uint64_t avail_end = avail_gpa + 4ULL + (2ULL * qsize);
    uint64_t used_gpa = vring_align(avail_end, vq->queue_align);

    vq->pfn = pfn;
    return virtio_queue_configure(vq, desc_gpa, avail_gpa, used_gpa);
}

/* Translate GPA range to Host Virtual Address with strict bounds check */
static void *gpa_to_hva(GuestMemory *mem, uint64_t gpa, uint32_t len) {
    if (!mem || !mem->hva_backing) return NULL;
    if (gpa >= mem->gpa_size || (gpa + (uint64_t)len) > mem->gpa_size) {
        return NULL; /* Out-of-bounds GPA */
    }
    return (void *)((uintptr_t)mem->hva_backing + (uintptr_t)gpa);
}

bool virtio_queue_has_available(VirtQueue *vq, GuestMemory *mem) {
    if (!vq || !vq->ready || !mem) return false;

    virtq_avail_t *avail = (virtq_avail_t *)gpa_to_hva(mem, vq->avail_gpa, sizeof(virtq_avail_t));
    if (!avail) return false;

    return (avail->idx != vq->last_avail_idx);
}

bool virtio_queue_pop_chain(VirtQueue *vq, GuestMemory *mem, VirtQueueChain *out_chain) {
    if (!vq || !vq->ready || !mem || !out_chain) return false;

    memset(out_chain, 0, sizeof(VirtQueueChain));

    /* 1. Map and check Available Ring */
    virtq_avail_t *avail = (virtq_avail_t *)gpa_to_hva(mem, vq->avail_gpa, sizeof(virtq_avail_t));
    if (!avail) return false;

    if (avail->idx == vq->last_avail_idx) {
        return false; /* No pending buffers */
    }

    /* 2. Read next available descriptor head index */
    uint16_t avail_slot = vq->last_avail_idx % vq->queue_size;
    uint64_t head_gpa = vq->avail_gpa + sizeof(virtq_avail_t) + (avail_slot * sizeof(uint16_t));
    uint16_t *head_ptr = (uint16_t *)gpa_to_hva(mem, head_gpa, sizeof(uint16_t));
    if (!head_ptr) return false;

    uint16_t desc_idx = *head_ptr;
    if (desc_idx >= vq->queue_size) {
        return false; /* Malformed head index */
    }

    out_chain->head_index = desc_idx;

    /* 3. Walk descriptor chain with loop detection */
    uint32_t count = 0;
    uint16_t cur_idx = desc_idx;
    bool visited[VIRTQUEUE_MAX_SIZE];
    memset(visited, 0, sizeof(visited));

    while (count < VIRTQUEUE_MAX_CHAIN_DESCRIPTORS) {
        if (cur_idx >= vq->queue_size || visited[cur_idx]) {
            return false; /* Invalid index or cycle detected */
        }
        visited[cur_idx] = true;

        /* Map Descriptor Entry */
        uint64_t desc_entry_gpa = vq->desc_gpa + ((uint64_t)cur_idx * sizeof(virtq_desc_t));
        virtq_desc_t *desc = (virtq_desc_t *)gpa_to_hva(mem, desc_entry_gpa, sizeof(virtq_desc_t));
        if (!desc) return false;

        /* Map Descriptor Buffer Payload */
        void *buf_hva = gpa_to_hva(mem, desc->addr, desc->len);
        if (!buf_hva && desc->len > 0) {
            return false; /* Descriptor points outside guest RAM */
        }

        VirtQueueBuffer *elem = &out_chain->buffers[count];
        elem->gpa = desc->addr;
        elem->hva = buf_hva;
        elem->len = desc->len;
        elem->flags = desc->flags;
        elem->is_write = (desc->flags & VIRTQ_DESC_F_WRITE) != 0;

        if (elem->is_write) {
            out_chain->total_writable_len += desc->len;
        } else {
            out_chain->total_readable_len += desc->len;
        }

        count++;

        if (!(desc->flags & VIRTQ_DESC_F_NEXT)) {
            break; /* Chain terminated normally */
        }

        cur_idx = desc->next;
    }

    out_chain->count = count;
    vq->last_avail_idx++;
    return true;
}

bool virtio_queue_complete_chain(VirtQueue *vq, GuestMemory *mem, uint32_t head_index, uint32_t written_len) {
    if (!vq || !vq->ready || !mem) return false;

    /* 1. Map Used Ring */
    virtq_used_t *used = (virtq_used_t *)gpa_to_hva(mem, vq->used_gpa, sizeof(virtq_used_t));
    if (!used) return false;

    /* 2. Write used element at current ring index */
    uint16_t used_slot = vq->last_used_idx % vq->queue_size;
    uint64_t elem_gpa = vq->used_gpa + sizeof(virtq_used_t) + (used_slot * sizeof(virtq_used_elem_t));
    virtq_used_elem_t *elem = (virtq_used_elem_t *)gpa_to_hva(mem, elem_gpa, sizeof(virtq_used_elem_t));
    if (!elem) return false;

    elem->id = head_index;
    elem->len = written_len;

    /* 3. Advance Used Ring Index (Memory Barrier guaranteed on x86) */
    vq->last_used_idx++;
    used->idx = vq->last_used_idx;

    return true;
}
