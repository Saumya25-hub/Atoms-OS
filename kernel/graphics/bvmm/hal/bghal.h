#ifndef _BOS_BVMM_BGHAL_H_
#define _BOS_BVMM_BGHAL_H_

#include "../defrag/bvmm_defrag.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bghal.h
 * @brief Production GPU Hardware Abstraction Layer (BGHAL V1.0) Master Header
 * 
 * BGHAL is the permanent vendor-neutral gateway between BVMM and physical GPU drivers.
 * Supports Intel (Xe/i915), AMD (AMDGPU), NVIDIA (Open GPU), VirtIO GPU, VMware SVGA II,
 * and Software Renderers.
 */

#define BGHAL_CANARY_MAGIC   0x42474841  /* "BGHA" */
#define BGHAL_MAX_GPUS       16
#define BGHAL_INVALID_GPU_ID 0ULL

typedef uint64_t bghal_gpu_id_t;

/* Vendor Classification Enums (Phase 10A Spec) */
typedef enum {
    BGHAL_VENDOR_UNKNOWN  = 0,
    BGHAL_VENDOR_INTEL    = 0x8086,
    BGHAL_VENDOR_AMD      = 0x1002,
    BGHAL_VENDOR_NVIDIA   = 0x10DE,
    BGHAL_VENDOR_VIRTIO   = 0x1AF4,
    BGHAL_VENDOR_VMWARE   = 0x15AD,
    BGHAL_VENDOR_SOFTWARE = 0xFFFF
} bghal_vendor_id_t;

/* PCI BAR Mapping Descriptor (Phase 10F Spec) */
typedef struct {
    uint64_t phys_addr;
    uint64_t virt_addr;
    uint64_t size_bytes;
    uint32_t bar_index;
    bool     is_mmio;
    bool     is_prefetchable;
} bghal_bar_desc_t;

/* Hardware Capability Matrix (Phase 10J Spec) */
typedef struct {
    uint32_t max_texture_dim;
    uint64_t max_single_alloc_bytes;
    uint64_t max_dma_transfer_bytes;
    uint32_t supported_queues;
    uint32_t supported_page_sizes;
    bool     has_hardware_timeline_fences;
    bool     has_bar_resize;
    bool     has_compression;
    bool     has_coherent_memory;
} bghal_capabilities_t;

/* GPU Page Table Descriptor (Phase 10E Spec) */
typedef struct {
    uint64_t gpu_virt_addr;
    uint64_t phys_vram_addr;
    uint64_t size_bytes;
    uint32_t flags;
    bool     is_mapped;
} bghal_page_table_t;

/* DMA Request Descriptor (Phase 10D Spec) */
typedef struct {
    uint64_t src_phys_addr;
    uint64_t dst_phys_addr;
    uint64_t size_bytes;
    uint32_t channel_id;
    bool     is_scatter_gather;
} bghal_dma_req_t;

/* Primary GPU Device Structure */
typedef struct bghal_gpu_device {
    uint32_t             canary_magic;   /* 0x42474841 */
    bghal_gpu_id_t       gpu_id;
    uint16_t             generation_id;
    uint32_t             registry_index;
    bghal_vendor_id_t    vendor_id;
    uint16_t             device_id;
    uint8_t              revision_id;
    uint64_t             total_vram_bytes;
    uint64_t             aperture_size_bytes;
    bghal_bar_desc_t     bars[6];
    bghal_capabilities_t caps;
    const struct bghal_backend_vtbl* vtbl;
    void*                vendor_priv;
    bool                 is_active;
} bghal_gpu_device_t;

/* Virtual Table Interface for Vendor Backends (Phase 10B Spec) */
typedef struct bghal_backend_vtbl {
    bvmm_result_t (*init)(bghal_gpu_device_t* gpu);
    bvmm_result_t (*shutdown)(bghal_gpu_device_t* gpu);
    bvmm_result_t (*allocate_vram)(bghal_gpu_device_t* gpu, uint64_t size_bytes, uint64_t* out_phys_addr);
    bvmm_result_t (*free_vram)(bghal_gpu_device_t* gpu, uint64_t phys_addr, uint64_t size_bytes);
    bvmm_result_t (*map_bar)(bghal_gpu_device_t* gpu, uint32_t bar_idx, uint64_t* out_virt_addr);
    bvmm_result_t (*unmap_bar)(bghal_gpu_device_t* gpu, uint32_t bar_idx);
    bvmm_result_t (*dma_copy)(bghal_gpu_device_t* gpu, const bghal_dma_req_t* dma_req);
    bvmm_result_t (*fence_signal)(bghal_gpu_device_t* gpu, uint64_t timeline_val);
    bvmm_result_t (*fence_wait)(bghal_gpu_device_t* gpu, uint64_t timeline_val, uint32_t timeout_ms);
    bvmm_result_t (*page_table_update)(bghal_gpu_device_t* gpu, const bghal_page_table_t* pt);
    bvmm_result_t (*cache_flush)(bghal_gpu_device_t* gpu);
    bvmm_result_t (*tlb_invalidate)(bghal_gpu_device_t* gpu);
    bvmm_result_t (*handle_interrupt)(bghal_gpu_device_t* gpu, uint32_t irq_vec);
    bvmm_result_t (*query_capabilities)(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps);
} bghal_backend_vtbl_t;

/* BGHAL Diagnostics Descriptor */
typedef struct {
    uint32_t total_gpus_detected;
    uint32_t active_gpus;
    uint64_t total_dma_copies;
    uint64_t total_bytes_dma_transferred;
    uint32_t total_page_table_updates;
    uint32_t cache_flushes;
    uint32_t tlb_invalidations;
    uint32_t interrupts_handled;
    uint32_t hardware_errors;
} bghal_diagnostics_t;

/**
 * @brief Initialize the BOS GPU Hardware Abstraction Layer (BGHAL V1.0).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_init(void);

/**
 * @brief Shutdown BGHAL and unregister all GPU devices.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_shutdown(void);

/**
 * @brief Discover physical GPU devices on PCI bus.
 * @param out_gpu_count Pointer to receive count of discovered GPUs.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_discover_gpus(uint32_t* out_gpu_count);

/**
 * @brief Retrieve primary GPU device handle.
 * @param out_gpu Pointer to receive GPU device pointer.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_get_primary_gpu(bghal_gpu_device_t** out_gpu);

/**
 * @brief Dispatch unified DMA copy across GPU hardware.
 * @param gpu Target GPU device.
 * @param req DMA request descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_dma_copy(bghal_gpu_device_t* gpu, const bghal_dma_req_t* req);

/**
 * @brief Update GPU virtual address page table mapping.
 * @param gpu Target GPU device.
 * @param pt Page table mapping descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_page_table_map(bghal_gpu_device_t* gpu, const bghal_page_table_t* pt);

/**
 * @brief Perform GPU cache flush and TLB invalidation.
 * @param gpu Target GPU device.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_cache_flush_and_tlb_invalidate(bghal_gpu_device_t* gpu);

/**
 * @brief Query GPU hardware capabilities.
 * @param gpu Target GPU device.
 * @param out_caps Pointer to receive hardware capabilities.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_query_capabilities(bghal_gpu_device_t* gpu, bghal_capabilities_t* out_caps);

/**
 * @brief Dump developer inspection log for BGHAL operations.
 */
void bghal_gpu_dump(void);

/**
 * @brief Retrieve current BGHAL diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t bghal_get_diagnostics(bghal_diagnostics_t* out_diag);

/**
 * @brief Validate internal integrity of BGHAL state.
 * @return true if valid, false if corruption detected.
 */
bool bghal_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_BGHAL_H_ */
