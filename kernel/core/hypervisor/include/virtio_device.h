/*
 * ATOMS OS — VirtIO Unified Device Model Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_DEVICE_H
#define ATOMS_VIRTIO_DEVICE_H

#include "virtio_types.h"
#include "virtio_queue.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VIRTIO_QUEUES_PER_DEV           8
#define VIRTIO_CONFIG_SPACE_MAX             256

struct atoms_vm;

/* VirtIO Device Base Object */
typedef struct virtio_dev {
    uint32_t device_id;
    uint16_t pci_device_id;
    uint8_t status;

    uint64_t host_features;
    uint64_t guest_features;
    uint32_t guest_features_sel;
    uint32_t host_features_sel;

    uint32_t num_queues;
    VirtQueue *queues[MAX_VIRTIO_QUEUES_PER_DEV];
    uint32_t queue_sel;

    uint8_t config_space[VIRTIO_CONFIG_SPACE_MAX];
    uint32_t config_len;

    uint8_t isr_status;  /* Interrupt Status Register (0x01: Queue, 0x02: Config) */
    uint8_t irq_line;

    struct atoms_vm *vm;
    void *backend_data;

    /* Device Lifecycle Callbacks */
    void (*on_reset)(struct virtio_dev *dev);
    void (*on_queue_notify)(struct virtio_dev *dev, uint32_t q_idx);
    void (*on_status_change)(struct virtio_dev *dev, uint8_t old_status, uint8_t new_status);
    void (*destroy)(struct virtio_dev *dev);
} VirtIODevice;

/* Device Generic Lifecycle APIs */
VirtIODevice *virtio_device_create(uint32_t dev_id, uint16_t pci_dev_id, uint32_t num_queues, uint32_t config_len);
void virtio_device_destroy(VirtIODevice *dev);
void virtio_device_reset(VirtIODevice *dev);

/* Status & Feature Negotiation */
void virtio_device_set_status(VirtIODevice *dev, uint8_t status);
bool virtio_device_negotiate_features(VirtIODevice *dev, uint64_t guest_features);

/* Interrupt Delivery Interface */
void virtio_device_raise_interrupt(VirtIODevice *dev, uint8_t isr_flag);
uint8_t virtio_device_read_isr(VirtIODevice *dev);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_DEVICE_H */
