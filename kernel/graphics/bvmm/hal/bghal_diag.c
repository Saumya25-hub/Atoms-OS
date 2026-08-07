#include "bghal.h"
#include "kernel/drivers/display/display.h"

void bghal_gpu_dump(void) {
    display_print("=== DPDP BGHAL HARDWARE ABSTRACTION LAYER DUMP ===\n");
    bghal_diagnostics_t diag;
    if (bghal_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BGHAL] GPUs Detected: ");
        display_print_dec(diag.total_gpus_detected);
        display_print(" | Active: ");
        display_print_dec(diag.active_gpus);
        display_print("\n[DPDP BGHAL] DMA Copies: ");
        display_print_dec((uint32_t)diag.total_dma_copies);
        display_print(" | Page Table Updates: ");
        display_print_dec(diag.total_page_table_updates);
        display_print("\n[DPDP BGHAL] Cache Flushes: ");
        display_print_dec(diag.cache_flushes);
        display_print(" | TLB Invalidations: ");
        display_print_dec(diag.tlb_invalidations);
        display_print("\n");
    }

    bghal_gpu_device_t* gpu = NULL;
    if (bghal_get_primary_gpu(&gpu) == BVMM_SUCCESS && gpu) {
        display_print("[DPDP BGHAL] Primary Vendor ID: 0x");
        display_print_hex(gpu->vendor_id);
        display_print(" | Device ID: 0x");
        display_print_hex(gpu->device_id);
        display_print(" | VRAM Size: ");
        display_print_dec((uint32_t)(gpu->total_vram_bytes / (1024 * 1024)));
        display_print(" MB\n");
    }
    display_print("==================================================\n");
}

bool bghal_validate_all(void) {
    bghal_diagnostics_t diag;
    if (bghal_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.hardware_errors == 0);
}
