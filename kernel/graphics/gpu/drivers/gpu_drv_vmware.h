#ifndef BOS_GPU_DRV_VMWARE_H
#define BOS_GPU_DRV_VMWARE_H

#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/include/gpu_svga_regs.h"

typedef struct vmware_svga_device {
    uint16_t io_base;            /* BAR0 IO Port Base */
    uint32_t svga_id;            /* Negotiated SVGA Version ID */
    
    /* BAR 1: Framebuffer */
    uint64_t fb_phys;            /* Physical base address */
    void*    fb_virt;            /* Virtual mapped base pointer */
    uint32_t fb_size;            /* Framebuffer total size in bytes */

    /* BAR 2: Command FIFO */
    uint64_t fifo_phys;          /* Physical FIFO base address */
    volatile uint32_t* fifo_virt;/* Virtual mapped FIFO base pointer */
    uint32_t fifo_size;          /* FIFO total size in bytes */
    uint32_t fifo_caps;          /* FIFO capabilities bitmask */

    /* Hardware Mode State */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
    uint32_t max_width;
    uint32_t max_height;

    /* Status Flags & Diagnostics */
    bool pci_bound;
    bool mmio_mapped;
    bool fifo_initialized;
    bool mode_set;
    uint32_t presents_count;
    uint32_t fillrect_count;
    uint32_t copy_count;
    uint32_t stretch_count;
} vmware_svga_device_t;

/* Public VMware SVGA driver registration & verification routines */
bos_gpu_status_t gpu_driver_vmware_register(void);
void             vmware_svga_dump_diagnostics(const bos_gpu_device_t* dev);
bool             vmware_svga_run_certification_suite(void);

#endif /* BOS_GPU_DRV_VMWARE_H */
