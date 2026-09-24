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

bool virtio_blk_read_sectors(VirtIOBlock *blk, uint64_t start_sector, uint32_t num_sectors, uint8_t *dest) {
    if (!blk || !dest) return false;
    if ((start_sector + num_sectors) > blk->total_sectors) return false;

    uint64_t sectors_left = num_sectors;
    uint64_t cur_sec = start_sector;
    uint8_t *cur_dest = dest;

    while (sectors_left > 0) {
        if (blk->chunk_table) {
            uint32_t chunk_idx = (uint32_t)(cur_sec / VIRTIO_BLK_SECTORS_PER_CHUNK);
            uint32_t chunk_sec_off = (uint32_t)(cur_sec % VIRTIO_BLK_SECTORS_PER_CHUNK);
            uint32_t chunk_sec_avail = (uint32_t)(VIRTIO_BLK_SECTORS_PER_CHUNK - chunk_sec_off);
            uint32_t to_transfer = (sectors_left < chunk_sec_avail) ? (uint32_t)sectors_left : chunk_sec_avail;
            uint32_t byte_count = to_transfer * 512;

            if (chunk_idx < blk->chunk_count && blk->chunk_table[chunk_idx]) {
                memcpy(cur_dest, blk->chunk_table[chunk_idx] + (chunk_sec_off * 512ULL), byte_count);
            } else {
                memset(cur_dest, 0, byte_count);
            }

            cur_dest += byte_count;
            cur_sec += to_transfer;
            sectors_left -= to_transfer;
        } else if (blk->storage_backing) {
            uint64_t byte_count = sectors_left * 512ULL;
            memcpy(cur_dest, blk->storage_backing + (cur_sec * 512ULL), (size_t)byte_count);
            break;
        } else {
            memset(cur_dest, 0, (size_t)(sectors_left * 512ULL));
            break;
        }
    }
    return true;
}

bool virtio_blk_write_sectors(VirtIOBlock *blk, uint64_t start_sector, uint32_t num_sectors, const uint8_t *src) {
    if (!blk || !src || blk->read_only) return false;
    if ((start_sector + num_sectors) > blk->total_sectors) return false;

    uint64_t sectors_left = num_sectors;
    uint64_t cur_sec = start_sector;
    const uint8_t *cur_src = src;

    while (sectors_left > 0) {
        if (blk->chunk_table) {
            uint32_t chunk_idx = (uint32_t)(cur_sec / VIRTIO_BLK_SECTORS_PER_CHUNK);
            uint32_t chunk_sec_off = (uint32_t)(cur_sec % VIRTIO_BLK_SECTORS_PER_CHUNK);
            uint32_t chunk_sec_avail = (uint32_t)(VIRTIO_BLK_SECTORS_PER_CHUNK - chunk_sec_off);
            uint32_t to_transfer = (sectors_left < chunk_sec_avail) ? (uint32_t)sectors_left : chunk_sec_avail;
            uint32_t byte_count = to_transfer * 512;

            if (chunk_idx >= blk->chunk_count) return false;

            if (!blk->chunk_table[chunk_idx]) {
                void *chunk = pmm_alloc_pages_nopanic(512);
                if (!chunk) return false;
                memset(chunk, 0, (size_t)VIRTIO_BLK_CHUNK_SIZE);
                blk->chunk_table[chunk_idx] = (uint8_t *)chunk;
                if (chunk_idx == 0 && !blk->storage_backing) {
                    blk->storage_backing = (uint8_t *)chunk;
                }
            }

            memcpy(blk->chunk_table[chunk_idx] + (chunk_sec_off * 512ULL), cur_src, byte_count);

            cur_src += byte_count;
            cur_sec += to_transfer;
            sectors_left -= to_transfer;
        } else if (blk->storage_backing) {
            uint64_t byte_count = sectors_left * 512ULL;
            memcpy(blk->storage_backing + (cur_sec * 512ULL), cur_src, (size_t)byte_count);
            break;
        } else {
            return false;
        }
    }
    return true;
}

bool virtio_blk_self_test(VirtIOBlock *blk) {
    if (!blk || blk->total_sectors == 0) return false;

    uint8_t test_buf[512];

    /* 1. Read beginning (Sector 0) */
    if (!virtio_blk_read_sectors(blk, 0, 1, test_buf)) {
        com1_puts("[VIRTIO-BLK TEST FAIL] Read at sector 0 (beginning) failed!\n");
        return false;
    }

    /* 2. Read middle */
    uint64_t mid_sec = blk->total_sectors / 2;
    if (!virtio_blk_read_sectors(blk, mid_sec, 1, test_buf)) {
        com1_puts("[VIRTIO-BLK TEST FAIL] Read at middle sector failed!\n");
        return false;
    }

    /* 3. Read end (last valid sector) */
    uint64_t end_sec = blk->total_sectors - 1;
    if (!virtio_blk_read_sectors(blk, end_sec, 1, test_buf)) {
        com1_puts("[VIRTIO-BLK TEST FAIL] Read at end sector failed!\n");
        return false;
    }

    /* 4. Out-of-range boundary check (Must be rejected) */
    if (virtio_blk_read_sectors(blk, blk->total_sectors, 1, test_buf)) {
        com1_puts("[VIRTIO-BLK TEST FAIL] Boundary check failed: Out-of-range read succeeded!\n");
        return false;
    }
    if (virtio_blk_read_sectors(blk, blk->total_sectors + 100, 1, test_buf)) {
        com1_puts("[VIRTIO-BLK TEST FAIL] Boundary check failed: Far out-of-range read succeeded!\n");
        return false;
    }

    com1_puts("[VIRTIO-BLK] Boundary Verification (Beginning, Middle, End, Out-of-Range Rejection): 100% PASS\n");
    return true;
}

VirtIOBlock *virtio_blk_create(uint64_t sector_count, bool read_only) {
    if (sector_count == 0) sector_count = VIRTIO_BLK_DEFAULT_SECTORS;

    VirtIOBlock *blk = (VirtIOBlock *)kmalloc(sizeof(VirtIOBlock));
    if (!blk) return NULL;
    memset(blk, 0, sizeof(VirtIOBlock));

    blk->total_sectors = sector_count;
    blk->read_only = read_only;

    if (sector_count >= 65536) {
        /* Large disk backing: allocate dynamic 2MB chunk table */
        uint32_t num_chunks = (uint32_t)((sector_count + VIRTIO_BLK_SECTORS_PER_CHUNK - 1) / VIRTIO_BLK_SECTORS_PER_CHUNK);
        blk->chunk_table = (uint8_t **)kmalloc(num_chunks * sizeof(uint8_t *));
        if (!blk->chunk_table) {
            kfree(blk);
            return NULL;
        }
        memset(blk->chunk_table, 0, num_chunks * sizeof(uint8_t *));
        blk->chunk_count = num_chunks;

        /* Pre-allocate chunk 0 (2 MB) for partition table and superblock */
        void *chunk0 = pmm_alloc_pages_nopanic(512);
        if (chunk0) {
            memset(chunk0, 0, (size_t)VIRTIO_BLK_CHUNK_SIZE);
            blk->chunk_table[0] = (uint8_t *)chunk0;
            blk->storage_backing = (uint8_t *)chunk0;
        }
    } else {
        /* Small disk: allocate contiguous RAM disk */
        uint64_t storage_bytes = sector_count * 512ULL;
        if (storage_bytes > (512 * 1024)) {
            size_t pages = (size_t)((storage_bytes + 4095) / 4096);
            blk->storage_backing = (uint8_t *)pmm_alloc_pages_nopanic(pages);
        } else {
            blk->storage_backing = (uint8_t *)kmalloc((size_t)storage_bytes);
        }
        if (!blk->storage_backing) {
            kfree(blk);
            return NULL;
        }
        memset(blk->storage_backing, 0, (size_t)storage_bytes);
    }

    /* Create underlying VirtIODevice (Device ID = 2 for Block, 1 Queue) */
    blk->base = virtio_device_create(VIRTIO_DEV_ID_BLOCK, VIRTIO_PCI_DEVICE_BLOCK, 1, sizeof(virtio_blk_config_t));
    if (!blk->base) {
        virtio_blk_destroy(blk);
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

    if (blk->chunk_table) {
        for (uint32_t i = 0; i < blk->chunk_count; i++) {
            if (blk->chunk_table[i]) {
                pmm_free_pages(blk->chunk_table[i], 512);
                blk->chunk_table[i] = NULL;
            }
        }
        kfree(blk->chunk_table);
        blk->chunk_table = NULL;
        blk->storage_backing = NULL;
    } else if (blk->storage_backing) {
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

                    uint32_t sectors_needed = data_buf->len / 512;
                    if (!virtio_blk_read_sectors(blk, cur_sector, sectors_needed, (uint8_t *)data_buf->hva)) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

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

                    uint32_t sectors_needed = data_buf->len / 512;
                    if (!virtio_blk_write_sectors(blk, cur_sector, sectors_needed, (const uint8_t *)data_buf->hva)) {
                        result_status = VIRTIO_BLK_S_IOERR;
                        break;
                    }

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
