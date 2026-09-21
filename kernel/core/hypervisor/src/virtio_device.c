/*
 * ATOMS OS — VirtIO Unified Device Model Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_device.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

VirtIODevice *virtio_device_create(uint32_t dev_id, uint16_t pci_dev_id, uint32_t num_queues, uint32_t config_len) {
    if (num_queues > MAX_VIRTIO_QUEUES_PER_DEV || config_len > VIRTIO_CONFIG_SPACE_MAX) {
        return NULL;
    }

    VirtIODevice *dev = (VirtIODevice *)kmalloc(sizeof(VirtIODevice));
    if (!dev) return NULL;
    memset(dev, 0, sizeof(VirtIODevice));

    dev->device_id = dev_id;
    dev->pci_device_id = pci_dev_id;
    dev->num_queues = num_queues;
    dev->config_len = config_len;
    dev->status = VIRTIO_STATUS_RESET;
    dev->isr_status = 0;
    dev->irq_line = 11; /* Default PCI IRQ Line */

    /* Allocate VirtQueues */
    for (uint32_t i = 0; i < num_queues; i++) {
        dev->queues[i] = virtio_queue_create(i, 128, 4096);
        if (!dev->queues[i]) {
            virtio_device_destroy(dev);
            return NULL;
        }
    }

    return dev;
}

void virtio_device_destroy(VirtIODevice *dev) {
    if (!dev) return;

    if (dev->destroy) {
        dev->destroy(dev);
    }

    for (uint32_t i = 0; i < dev->num_queues; i++) {
        if (dev->queues[i]) {
            virtio_queue_destroy(dev->queues[i]);
            dev->queues[i] = NULL;
        }
    }

    kfree(dev);
}

void virtio_device_reset(VirtIODevice *dev) {
    if (!dev) return;

    dev->status = VIRTIO_STATUS_RESET;
    dev->guest_features = 0;
    dev->guest_features_sel = 0;
    dev->host_features_sel = 0;
    dev->queue_sel = 0;
    dev->isr_status = 0;

    for (uint32_t i = 0; i < dev->num_queues; i++) {
        if (dev->queues[i]) {
            virtio_queue_reset(dev->queues[i]);
        }
    }

    if (dev->on_reset) {
        dev->on_reset(dev);
    }
}

void virtio_device_set_status(VirtIODevice *dev, uint8_t new_status) {
    if (!dev) return;

    uint8_t old_status = dev->status;

    if (new_status == VIRTIO_STATUS_RESET) {
        virtio_device_reset(dev);
        return;
    }

    dev->status = new_status;

    if (dev->on_status_change) {
        dev->on_status_change(dev, old_status, new_status);
    }
}

bool virtio_device_negotiate_features(VirtIODevice *dev, uint64_t guest_features) {
    if (!dev) return false;

    /* Accept only features supported by the host backend */
    dev->guest_features = guest_features & dev->host_features;
    return true;
}

void virtio_device_raise_interrupt(VirtIODevice *dev, uint8_t isr_flag) {
    if (!dev) return;

    dev->isr_status |= isr_flag;

    /* In a full VM environment, inject virtual interrupt vector into guest vCPU */
    if (dev->vm && dev->vm->bsp_vcpu) {
        /* Set interrupt pending flag */
    }
}

uint8_t virtio_device_read_isr(VirtIODevice *dev) {
    if (!dev) return 0;

    uint8_t ret = dev->isr_status;
    dev->isr_status = 0; /* Auto-clear upon read (standard VirtIO PCI spec) */
    return ret;
}
