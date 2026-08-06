#ifndef BOS_GPU_DRV_NVIDIA_H
#define BOS_GPU_DRV_NVIDIA_H

#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/include/gpu_nvidia_regs.h"

typedef struct nvidia_gpu_device {
    /* BAR 0: MMIO Register Space */
    uint64_t mmio_phys;
    uint32_t mmio_size;
    volatile uint32_t* mmio_virt;

    /* BAR 1: VRAM Aperture Window */
    uint64_t vram_phys;
    uint32_t vram_size;
    void*    vram_virt;

    /* Hardware Family & Device Meta */
    uint16_t device_id;
    const char* family_name;
    bool pci_bound;
    bool mmio_mapped;
    bool power_enabled;
    bool display_active;
    bool cursor_active;

    /* Display Pipe State */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;

    /* Diagnostic Counters */
    uint32_t presents_count;
    uint32_t fillrect_count;
    uint32_t copy_count;
    uint32_t stretch_count;
    uint32_t flip_count;
} nvidia_gpu_device_t;

/* Public NVIDIA GPU driver registration & verification routines */
bos_gpu_status_t gpu_driver_nvidia_register(void);
void             nvidia_gpu_dump_diagnostics(const bos_gpu_device_t* dev);
bool             nvidia_gpu_run_certification_suite(void);

#endif /* BOS_GPU_DRV_NVIDIA_H */
