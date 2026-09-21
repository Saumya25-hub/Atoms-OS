/*
 * ATOMS OS — VirtIO PCI Transport Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 3: VirtIO Virtual Hardware Subsystem
 */

#include "kernel/core/hypervisor/include/virtio_pci.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

VirtualPCIBus *virtual_pci_bus_create(void) {
    VirtualPCIBus *bus = (VirtualPCIBus *)kmalloc(sizeof(VirtualPCIBus));
    if (!bus) return NULL;
    memset(bus, 0, sizeof(VirtualPCIBus));

    bus->next_io_base = 0xC000;          /* Standard Virtual I/O Base */
    bus->next_mmio_base = 0xFEB00000ULL; /* Standard Virtual 32-bit MMIO Base */
    bus->device_count = 0;

    return bus;
}

void virtual_pci_bus_destroy(VirtualPCIBus *bus) {
    if (!bus) return;

    for (uint32_t i = 0; i < bus->device_count; i++) {
        if (bus->devices[i]) {
            kfree(bus->devices[i]);
            bus->devices[i] = NULL;
        }
    }

    kfree(bus);
}

VirtIOPCIDevice *virtual_pci_register_virtio_device(VirtualPCIBus *bus, VirtIODevice *vdev, uint8_t base_class, uint8_t sub_class) {
    if (!bus || !vdev || bus->device_count >= MAX_VIRTUAL_PCI_DEVICES) {
        return NULL;
    }

    VirtIOPCIDevice *pdev = (VirtIOPCIDevice *)kmalloc(sizeof(VirtIOPCIDevice));
    if (!pdev) return NULL;
    memset(pdev, 0, sizeof(VirtIOPCIDevice));

    pdev->bus = 0;
    pdev->slot = (uint8_t)(bus->device_count + 1); /* Slot 1, 2, 3... */
    pdev->func = 0;
    pdev->vdev = vdev;

    /* Assign I/O Port BAR0 (64 bytes aligned) */
    pdev->io_bar_base = bus->next_io_base;
    pdev->io_bar_size = 64;
    bus->next_io_base += 64;

    /* Assign MMIO BAR1 (4KB page aligned) */
    pdev->mmio_bar_base = bus->next_mmio_base;
    pdev->mmio_bar_size = 4096;
    bus->next_mmio_base += 4096;

    /* Populate Standard Type 0 PCI Configuration Space Header */
    uint8_t *cfg = pdev->pci_config;
    *(uint16_t *)(cfg + 0x00) = VIRTIO_PCI_VENDOR_ID;   /* Vendor ID: 0x1AF4 (Red Hat / VirtIO) */
    *(uint16_t *)(cfg + 0x02) = vdev->pci_device_id;     /* Device ID */
    *(uint16_t *)(cfg + 0x04) = 0x0007;                 /* Command: I/O + Memory + BusMaster */
    *(uint16_t *)(cfg + 0x06) = 0x0010;                 /* Status: Capabilities List */
    *(uint8_t  *)(cfg + 0x08) = 0x00;                   /* Revision ID */
    *(uint8_t  *)(cfg + 0x09) = 0x00;                   /* Prog IF */
    *(uint8_t  *)(cfg + 0x0A) = sub_class;              /* Subclass */
    *(uint8_t  *)(cfg + 0x0B) = base_class;             /* Base Class */
    *(uint8_t  *)(cfg + 0x0E) = 0x00;                   /* Header Type 0 (Standard Endpoint) */

    /* BAR0: I/O Port Address (Bit 0 = 1) */
    *(uint32_t *)(cfg + 0x10) = ((uint32_t)pdev->io_bar_base) | 0x01;

    /* BAR1: 32-bit MMIO Address (Bit 0 = 0) */
    *(uint32_t *)(cfg + 0x14) = (uint32_t)pdev->mmio_bar_base;

    /* Subsystem Identifiers */
    *(uint16_t *)(cfg + 0x2C) = VIRTIO_PCI_VENDOR_ID;
    *(uint16_t *)(cfg + 0x2E) = vdev->pci_device_id;

    /* Interrupt Configuration */
    *(uint8_t  *)(cfg + 0x3C) = vdev->irq_line;         /* Interrupt Line (IRQ 11) */
    *(uint8_t  *)(cfg + 0x3D) = 0x01;                   /* Interrupt Pin (INTA#) */

    bus->devices[bus->device_count++] = pdev;
    return pdev;
}

uint32_t virtual_pci_config_read(VirtualPCIBus *bus, uint8_t bus_num, uint8_t slot, uint8_t func, uint8_t offset, uint8_t size) {
    if (!bus || bus_num != 0) return 0xFFFFFFFF;

    for (uint32_t i = 0; i < bus->device_count; i++) {
        VirtIOPCIDevice *pdev = bus->devices[i];
        if (pdev && pdev->slot == slot && pdev->func == func) {
            if ((uint32_t)offset + (uint32_t)size > 256) return 0xFFFFFFFF;

            if (size == 1) {
                return pdev->pci_config[offset];
            } else if (size == 2 && (offset + 1) < 256) {
                return *(uint16_t *)(pdev->pci_config + offset);
            } else if (size == 4 && (offset + 3) < 256) {
                return *(uint32_t *)(pdev->pci_config + offset);
            }
        }
    }

    return 0xFFFFFFFF; /* Device Not Present */
}

void virtual_pci_config_write(VirtualPCIBus *bus, uint8_t bus_num, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val, uint8_t size) {
    if (!bus || bus_num != 0) return;

    for (uint32_t i = 0; i < bus->device_count; i++) {
        VirtIOPCIDevice *pdev = bus->devices[i];
        if (pdev && pdev->slot == slot && pdev->func == func) {
            if ((uint32_t)offset + (uint32_t)size > 256) return;

            /* Allow writes to Command register (0x04) and BAR sizing probes */
            if (offset == 0x04) {
                pdev->pci_config[0x04] = (uint8_t)val;
            } else if (offset == 0x10 && val == 0xFFFFFFFF) {
                /* BAR0 Size Probe: Return ~(io_bar_size - 1) | 1 */
                *(uint32_t *)(pdev->pci_config + 0x10) = (~((uint32_t)pdev->io_bar_size - 1)) | 0x01;
            } else if (offset == 0x10) {
                /* Restore BAR0 address */
                *(uint32_t *)(pdev->pci_config + 0x10) = (val & ~0x3) | 0x01;
                pdev->io_bar_base = (uint16_t)(val & ~0x3);
            }
        }
    }
}

uint32_t virtio_pci_bar_read(VirtIOPCIDevice *pci_dev, uint32_t offset, uint8_t size) {
    if (!pci_dev || !pci_dev->vdev) return 0;
    VirtIODevice *vdev = pci_dev->vdev;

    switch (offset) {
        case VIRTIO_PCI_HOST_FEATURES:
            return (uint32_t)vdev->host_features;

        case VIRTIO_PCI_QUEUE_PFN:
            if (vdev->queue_sel < vdev->num_queues && vdev->queues[vdev->queue_sel]) {
                return vdev->queues[vdev->queue_sel]->pfn;
            }
            return 0;

        case VIRTIO_PCI_QUEUE_NUM:
            if (vdev->queue_sel < vdev->num_queues && vdev->queues[vdev->queue_sel]) {
                return vdev->queues[vdev->queue_sel]->queue_size;
            }
            return 0;

        case VIRTIO_PCI_QUEUE_SEL:
            return (uint16_t)vdev->queue_sel;

        case VIRTIO_PCI_STATUS:
            return vdev->status;

        case VIRTIO_PCI_ISR:
            return virtio_device_read_isr(vdev);

        default:
            /* Device-Specific Configuration Space (offset >= 0x14) */
            if (offset >= VIRTIO_PCI_CONFIG_OFF) {
                uint32_t cfg_off = offset - VIRTIO_PCI_CONFIG_OFF;
                if (cfg_off < vdev->config_len) {
                    if (size == 1) {
                        return vdev->config_space[cfg_off];
                    } else if (size == 2 && (cfg_off + 1) < vdev->config_len) {
                        return *(uint16_t *)(vdev->config_space + cfg_off);
                    } else if (size == 4 && (cfg_off + 3) < vdev->config_len) {
                        return *(uint32_t *)(vdev->config_space + cfg_off);
                    }
                }
            }
            break;
    }

    return 0;
}

void virtio_pci_bar_write(VirtIOPCIDevice *pci_dev, uint32_t offset, uint32_t val, uint8_t size) {
    if (!pci_dev || !pci_dev->vdev) return;
    VirtIODevice *vdev = pci_dev->vdev;

    switch (offset) {
        case VIRTIO_PCI_GUEST_FEATURES:
            virtio_device_negotiate_features(vdev, (uint64_t)val);
            break;

        case VIRTIO_PCI_QUEUE_PFN:
            if (vdev->queue_sel < vdev->num_queues && vdev->queues[vdev->queue_sel]) {
                virtio_queue_set_pfn(vdev->queues[vdev->queue_sel], val, 4096);
            }
            break;

        case VIRTIO_PCI_QUEUE_SEL:
            vdev->queue_sel = (uint16_t)val;
            break;

        case VIRTIO_PCI_QUEUE_NOTIFY: {
            uint16_t q_idx = (uint16_t)val;
            if (q_idx < vdev->num_queues && vdev->on_queue_notify) {
                vdev->on_queue_notify(vdev, q_idx);
            }
            break;
        }

        case VIRTIO_PCI_STATUS:
            virtio_device_set_status(vdev, (uint8_t)val);
            break;

        default:
            /* Device-Specific Configuration Space (offset >= 0x14) */
            if (offset >= VIRTIO_PCI_CONFIG_OFF) {
                uint32_t cfg_off = offset - VIRTIO_PCI_CONFIG_OFF;
                if (cfg_off < vdev->config_len) {
                    if (size == 1) {
                        vdev->config_space[cfg_off] = (uint8_t)val;
                    } else if (size == 2 && (cfg_off + 1) < vdev->config_len) {
                        *(uint16_t *)(vdev->config_space + cfg_off) = (uint16_t)val;
                    } else if (size == 4 && (cfg_off + 3) < vdev->config_len) {
                        *(uint32_t *)(vdev->config_space + cfg_off) = val;
                    }
                }
            }
            break;
    }
}
