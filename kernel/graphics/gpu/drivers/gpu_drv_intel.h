#ifndef BOS_GPU_DRV_INTEL_H
#define BOS_GPU_DRV_INTEL_H

#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/graphics/gpu/include/gpu_intel_regs.h"

typedef struct intel_gpu_device {
    /* BAR 0: GTTMMADR (MMIO + GGTT PTEs) */
    uint64_t mmio_phys;
    uint32_t mmio_size;
    volatile uint32_t* mmio_virt;
    volatile uint64_t* ggtt_virt;   /* Pointer to GGTT PTE Table at mmio_virt + 0x800000 */

    /* BAR 2: GMADR (Graphics Aperture Window) */
    uint64_t gmadr_phys;
    uint32_t gmadr_size;
    void*    gmadr_virt;

    /* Stolen Memory (VRAM Base from BSM) */
    uint64_t stolen_phys;
    uint32_t stolen_size;

    /* Hardware Generation & Device Meta */
    uint32_t gen;                   /* Gen6, Gen7, Gen8, Gen9, Gen11, Gen12 */
    uint16_t device_id;
    bool pci_bound;
    bool mmio_mapped;
    bool power_enabled;
    bool pipe_active;
    bool cursor_active;

    /* Display Pipe State */
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;

    /* Diagnostic counters */
    uint32_t presents_count;
    uint32_t fillrect_count;
    uint32_t copy_count;
    uint32_t stretch_count;
    uint32_t flip_count;
} intel_gpu_device_t;

/* Public Intel GPU driver registration & verification routines */
bos_gpu_status_t gpu_driver_intel_register(void);
void             intel_gpu_dump_diagnostics(const bos_gpu_device_t* dev);
bool             intel_gpu_run_certification_suite(void);

#endif /* BOS_GPU_DRV_INTEL_H */
