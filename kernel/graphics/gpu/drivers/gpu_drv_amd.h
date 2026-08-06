#ifndef BOS_GPU_DRV_AMD_H
#define BOS_GPU_DRV_AMD_H

#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/include/gpu_amd_regs.h"

#define AMD_SDMA_RING_SIZE                  0x1000

typedef struct amd_gpu_device {
    /* BAR 0: MMIO Register Space */
    uint64_t mmio_phys;
    uint32_t mmio_size;
    volatile uint32_t* mmio_virt;

    /* BAR 2: VRAM Aperture Window */
    uint64_t vram_phys;
    uint32_t vram_size;
    void*    vram_virt;

    /* SDMA Engine Ring Buffer */
    uint64_t sdma_ring_phys;
    volatile uint32_t* sdma_ring_virt;
    uint32_t sdma_wptr;
    uint32_t sdma_rptr;

    /* GART Page Table */
    uint64_t gart_phys;
    volatile uint64_t* gart_virt;
    uint32_t gart_size;

    /* Hardware Family & Device Meta */
    uint16_t device_id;
    const char* family_name;
    bool pci_bound;
    bool mmio_mapped;
    bool sdma_initialized;
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
    uint32_t sdma_cmd_count;
} amd_gpu_device_t;

/* Public AMD Radeon GPU driver registration & verification routines */
bos_gpu_status_t gpu_driver_amd_register(void);
void             amd_gpu_dump_diagnostics(const bos_gpu_device_t* dev);
bool             amd_gpu_run_certification_suite(void);

#endif /* BOS_GPU_DRV_AMD_H */
