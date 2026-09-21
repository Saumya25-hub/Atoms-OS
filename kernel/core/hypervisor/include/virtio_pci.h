/*
 * ATOMS OS — VirtIO PCI Transport & Emulated Virtual PCI Device Layer
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#ifndef ATOMS_VIRTIO_PCI_H
#define ATOMS_VIRTIO_PCI_H

#include "virtio_device.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VirtIO PCI Legacy Register Offsets in BAR0 */
#define VIRTIO_PCI_HOST_FEATURES            0x00    /* 32-bit R */
#define VIRTIO_PCI_GUEST_FEATURES           0x04    /* 32-bit W */
#define VIRTIO_PCI_QUEUE_PFN                0x08    /* 32-bit RW */
#define VIRTIO_PCI_QUEUE_NUM                0x0C    /* 16-bit R */
#define VIRTIO_PCI_QUEUE_SEL                0x0E    /* 16-bit RW */
#define VIRTIO_PCI_QUEUE_NOTIFY             0x10    /* 16-bit W */
#define VIRTIO_PCI_STATUS                   0x12    /* 8-bit RW */
#define VIRTIO_PCI_ISR                      0x13    /* 8-bit R (read-to-clear) */
#define VIRTIO_PCI_CONFIG_OFF               0x14    /* Start of Device Config */

#define MAX_VIRTUAL_PCI_DEVICES             16

/* Virtual PCI Device Header */
typedef struct virtio_pci_dev {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;

    /* 256-byte Standard PCI Configuration Space */
    uint8_t pci_config[256];

    uint16_t io_bar_base;
    uint16_t io_bar_size;
    uint64_t mmio_bar_base;
    uint64_t mmio_bar_size;

    VirtIODevice *vdev;
} VirtIOPCIDevice;

/* Virtual PCI Bus Interface */
typedef struct VirtualPCIBus VirtualPCIBus;
struct VirtualPCIBus {
    uint32_t device_count;
    VirtIOPCIDevice *devices[MAX_VIRTUAL_PCI_DEVICES];
    uint16_t next_io_base;
    uint64_t next_mmio_base;
};

/* Virtual PCI Core APIs */
VirtualPCIBus *virtual_pci_bus_create(void);
void virtual_pci_bus_destroy(VirtualPCIBus *bus);
VirtIOPCIDevice *virtual_pci_register_virtio_device(VirtualPCIBus *bus, VirtIODevice *vdev, uint8_t base_class, uint8_t sub_class);

/* PCI Configuration Space Access Dispatch */
uint32_t virtual_pci_config_read(VirtualPCIBus *bus, uint8_t bus_num, uint8_t slot, uint8_t func, uint8_t offset, uint8_t size);
void virtual_pci_config_write(VirtualPCIBus *bus, uint8_t bus_num, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val, uint8_t size);

/* VirtIO BAR I/O & MMIO Emulation Dispatch */
uint32_t virtio_pci_bar_read(VirtIOPCIDevice *pci_dev, uint32_t offset, uint8_t size);
void virtio_pci_bar_write(VirtIOPCIDevice *pci_dev, uint32_t offset, uint32_t val, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTIO_PCI_H */
