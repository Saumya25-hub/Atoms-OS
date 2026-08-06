#ifndef BOS_GPU_DEVICE_H
#define BOS_GPU_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/graphics/gpu/include/gpu_caps.h"

/* Known GPU PCI Vendor IDs */
#define BOS_GPU_VENDOR_VMWARE    0x15AD
#define BOS_GPU_VENDOR_VIRTIO    0x1AF4
#define BOS_GPU_VENDOR_INTEL     0x8086
#define BOS_GPU_VENDOR_AMD       0x1002
#define BOS_GPU_VENDOR_NVIDIA    0x10DE
#define BOS_GPU_VENDOR_SOFTWARE  0xFFFF

/* BAR Types */
typedef enum {
    BOS_GPU_BAR_TYPE_UNUSED = 0,
    BOS_GPU_BAR_TYPE_MMIO32,
    BOS_GPU_BAR_TYPE_MMIO64,
    BOS_GPU_BAR_TYPE_IO
} bos_gpu_bar_type_t;

/* BAR Info */
typedef struct {
    uint8_t index;
    bos_gpu_bar_type_t type;
    uint64_t base_address;
    uint64_t size;
    bool prefetchable;
} bos_gpu_bar_info_t;

/* Forward declaration for driver ops */
struct bos_gpu_driver_ops;

/* GPU Device Structure */
typedef struct bos_gpu_device {
    uint32_t device_id_idx;           /* Subsystem internal Index */
    uint16_t vendor_id;
    uint16_t device_id;
    char name[64];
    char driver_name[64];
    uint64_t capabilities;
    uint64_t vram_size;
    bos_gpu_bar_info_t bars[6];
    
    /* PCI BDF location */
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint8_t irq;

    /* Drivers function table */
    const struct bos_gpu_driver_ops* ops;

    /* Driver internal state */
    void* private_data;

    /* Device Status */
    bool is_initialized;
    bool is_active;
    bool is_primary;
} bos_gpu_device_t;

#endif /* BOS_GPU_DEVICE_H */
