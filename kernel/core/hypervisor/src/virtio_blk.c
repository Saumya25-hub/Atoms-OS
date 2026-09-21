/*
 * ATOMS OS — VirtIO Block Device Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_blk.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static void blk_queue_notify_cb(VirtIODevice *dev, uint32_t q_idx) {
    if (!dev || q_idx != 0) return;
    VirtIOBlock *blk = (VirtIOBlock *)dev->backend_data;
    if (blk) {
        virtio_blk_process_queue(blk);
    }
}

static void blk_reset_cb(VirtIODevice *dev) {
    /* No persistent hardware state to clear on RAM disk reset */
    (void)dev;
}

VirtIOBlock *virtio_blk_create(uint64_t sector_count, bool read_only) {
    if (sector_count == 0) sector_count = VIRTIO_BLK_DEFAULT_SECTORS;

    VirtIOBlock *blk = (VirtIOBlock *)kmalloc(sizeof(VirtIOBlock));
    if (!blk) return NULL;
    memset(blk, 0, sizeof(VirtIOBlock));

    blk->total_sectors = sector_count;
    blk->read_only = read_only;

    /* Allocate RAM Disk Storage Backing (512 bytes per sector) */
    uint64_t storage_bytes = sector_count * 512ULL;
    if (storage_bytes > (512 * 1024)) {
        size_t pages = (size_t)((storage_bytes + 4095) / 4096);
        blk->storage_backing = (uint8_t *)pmm_alloc_pages(pages);
    } else {
        blk->storage_backing = (uint8_t *)kmalloc((size_t)storage_bytes);
    }
    if (!blk->storage_backing) {
        kfree(blk);
        return NULL;
    }
    memset(blk->storage_backing, 0, (size_t)storage_bytes);

    /* Create underlying VirtIODevice (Device ID = 2 for Block, 1 Queue) */
    blk->base = virtio_device_create(VIRTIO_DEV_ID_BLOCK, VIRTIO_PCI_DEVICE_BLOCK, 1, sizeof(virtio_blk_config_t));
    if (!blk->base) {
        if (storage_bytes > (512 * 1024)) {
            size_t pages = (size_t)((storage_bytes + 4095) / 4096);
            pmm_free_pages(blk->storage_backing, pages);
        } else {
            kfree(blk->storage_backing);
        }
        kfree(blk);
        return NULL;
    }

    blk->base->backend_data = blk;
    blk->base->on_queue_notify = blk_queue_notify_cb;
    blk->base->on_reset = blk_reset_cb;

    /* Configure Supported Host Features */
    blk->base->host_features = VIRTIO_BLK_F_FLUSH | VIRTIO_BLK_F_BLK_SIZE | VIRTIO_F_VERSION_1;
    if (read_only) {
        blk->base->host_features |= VIRTIO_BLK_F_RO;
    }

    /* Populate Configuration Space */
    virtio_blk_config_t *cfg = (virtio_blk_config_t *)blk->base->config_space;
    cfg->capacity = sector_count;
    cfg->blk_size = 512;
    cfg->seg_max = 128;
    cfg->size_max = 65536;

    return blk;
}

void virtio_blk_destroy(VirtIOBlock *blk) {
    if (!blk) return;

    if (blk->base) {
        virtio_device_destroy(blk->base);
        blk->base = NULL;
    }

    if (blk->storage_backing) {
        uint64_t storage_bytes = blk->total_sectors * 512ULL;
        if (storage_bytes > (512 * 1024)) {
            size_t pages = (size_t)((storage_bytes + 4095) / 4096);
            pmm_free_pages(blk->storage_backing, pages);
        } else {
            kfree(blk->storage_backing);
        }
        blk->storage_backing = NULL;
    }

    kfree(blk);
}

void virtio_blk_process_queue(VirtIOBlock *blk) {
    if (!blk || !blk->base || !blk->base->vm || !blk->base->vm->guest_mem) return;

    VirtIODevice *dev = blk->base;
    VirtQueue *vq = dev->queues[0];
    GuestMemory *mem = dev->vm->guest_mem;

    VirtQueueChain chain;
    while (virtio_queue_pop_chain(vq, mem, &chain)) {
        if (chain.count < 2) {
            /* Malformed: Must have at least request header and status byte */
            continue;
        }

        /* 1. First buffer is Request Header */
        VirtQueueBuffer *hdr_buf = &chain.buffers[0];
        if (hdr_buf->len < sizeof(virtio_blk_req_hdr_t) || !hdr_buf->hva) {
            continue;
        }
        virtio_blk_req_hdr_t *req = (virtio_blk_req_hdr_t *)hdr_buf->hva;

        /* 2. Last buffer is Status Byte (must be writable by host) */
        VirtQueueBuffer *status_buf = &chain.buffers[chain.count - 1];
        if (status_buf->len < 1 || !status_buf->hva || !status_buf->is_write) {
            continue;
        }
        uint8_t *status_ptr = (uint8_t *)status_buf->hva;

        uint32_t bytes_transferred = 0;
        uint8_t result_status = VIRTIO_BLK_S_OK;

        switch (req->type) {
            case VIRTIO_BLK_T_IN: { /* READ */
                blk->total_reads++;
                uint64_t cur_sector = req->sector;

                for (uint32_t i = 1; i < chain.count - 1; i++) {
                    VirtQueueBuffer *data_buf = &chain.buffers[i];
                    if (!data_buf->is_write || !data_buf->hva) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

                    uint64_t sectors_needed = data_buf->len / 512;
                    if ((cur_sector + sectors_needed) > blk->total_sectors) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

                    uint64_t offset = cur_sector * 512ULL;
                    memcpy(data_buf->hva, blk->storage_backing + offset, data_buf->len);
                    bytes_transferred += data_buf->len;
                    cur_sector += sectors_needed;
                }
                blk->total_bytes_read += bytes_transferred;
                break;
            }

            case VIRTIO_BLK_T_OUT: { /* WRITE */
                blk->total_writes++;
                if (blk->read_only) {
                    result_status = VIRTIO_BLK_S_IOERR;
                    break;
                }

                uint64_t cur_sector = req->sector;
                for (uint32_t i = 1; i < chain.count - 1; i++) {
                    VirtQueueBuffer *data_buf = &chain.buffers[i];
                    if (data_buf->is_write || !data_buf->hva) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

                    uint64_t sectors_needed = data_buf->len / 512;
                    if ((cur_sector + sectors_needed) > blk->total_sectors) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

                    uint64_t offset = cur_sector * 512ULL;
                    memcpy(blk->storage_backing + offset, data_buf->hva, data_buf->len);
                    bytes_transferred += data_buf->len;
                    cur_sector += sectors_needed;
                }
                blk->total_bytes_written += bytes_transferred;
                break;
            }

            case VIRTIO_BLK_T_FLUSH: {
                blk->total_flushes++;
                result_status = VIRTIO_BLK_S_OK;
                break;
            }

            case VIRTIO_BLK_T_GET_ID: {
                if (chain.count >= 3 && chain.buffers[1].is_write && chain.buffers[1].hva) {
                    const char *serial = VIRTIO_BLK_SERIAL;
                    size_t slen = strlen(serial);
                    if (chain.buffers[1].len < slen) slen = chain.buffers[1].len;
                    memcpy(chain.buffers[1].hva, serial, slen);
                    bytes_transferred = (uint32_t)slen;
                    result_status = VIRTIO_BLK_S_OK;
                } else {
                    result_status = VIRTIO_BLK_S_UNSUPP;
                }
                break;
            }

            default:
                result_status = VIRTIO_BLK_S_UNSUPP;
                break;
        }

        /* 3. Write Status Byte */
        *status_ptr = result_status;

        /* 4. Complete Chain in Used Ring */
        virtio_queue_complete_chain(vq, mem, chain.head_index, bytes_transferred + 1);

        /* 5. Trigger Queue Interrupt */
        virtio_device_raise_interrupt(dev, 0x01);
    }
}
