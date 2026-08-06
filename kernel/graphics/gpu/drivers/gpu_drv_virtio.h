#ifndef BOS_GPU_DRV_VIRTIO_H
#define BOS_GPU_DRV_VIRTIO_H

#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/include/gpu_virtio_regs.h"

#define VIRTIO_GPU_QUEUE_SIZE               64
#define VIRTIO_GPU_MAX_RESOURCES            16

typedef struct virtio_gpu_queue {
    uint32_t num;
    uint32_t num_max;
    
    virtq_desc_t*   desc;            /* Descriptor Array */
    virtq_avail_t*  avail;           /* Available Ring */
    virtq_used_t*   used;            /* Used Ring */

    uint16_t last_used_idx;
    uint16_t free_head;
    uint16_t num_free;
} virtio_gpu_queue_t;

typedef struct virtio_gpu_device {
    uint64_t mmio_base;          /* MMIO BAR Base Address */
    uint32_t mmio_size;
    volatile uint32_t* mmio_virt;/* Virtual Mapped MMIO Pointer */
    uint16_t io_base;            /* PCI Legacy IO Base Address */
    bool     is_pci_io;          /* True if using PCI Legacy IO ports */

    /* VirtQueue 0: Control Queue */
    virtio_gpu_queue_t ctrl_queue;
    
    /* Device Negotiation Status */
    uint32_t device_features;
    uint32_t status;
    bool pci_bound;
    bool mmio_mapped;
    bool queues_initialized;
    bool scanout_active;

    /* Screen & Scanout State */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    uint32_t active_resource_id;

    /* Backing Framebuffer Pointer */
    uint64_t fb_phys;
    void*    fb_virt;
    uint32_t fb_size;

    /* Diagnostic counters */
    uint32_t presents_count;
    uint32_t fillrect_count;
    uint32_t copy_count;
    uint32_t stretch_count;
    uint32_t cmd_count;
} virtio_gpu_device_t;

/* Public VirtIO GPU driver registration & verification routines */
bos_gpu_status_t gpu_driver_virtio_register(void);
void             virtio_gpu_dump_diagnostics(const bos_gpu_device_t* dev);
bool             virtio_gpu_run_certification_suite(void);

#endif /* BOS_GPU_DRV_VIRTIO_H */
