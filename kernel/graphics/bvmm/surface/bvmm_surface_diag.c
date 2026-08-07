#include "bvmm_surface.h"
#include "kernel/drivers/display/display.h"

extern bvmm_result_t bvmm_surface_registry_lookup(bvmm_surface_id_t id, bvmm_surface_desc_t** out_desc);
extern uint32_t bvmm_surface_registry_get_active_count(void);

void bvmm_surface_dump(bvmm_surface_id_t surface_id) {
    bvmm_surface_desc_t* desc = NULL;
    if (bvmm_surface_registry_lookup(surface_id, &desc) != BVMM_SUCCESS || !desc) {
        display_print("[BSME INSPECTOR] Surface ID ");
        display_print_dec((uint32_t)surface_id);
        display_print(" NOT FOUND\n");
        return;
    }

    display_print("[BSME INSPECTOR] Surface ID: ");
    display_print_dec((uint32_t)desc->surface_id);
    display_print(" | Resolution: ");
    display_print_dec(desc->width);
    display_print("x");
    display_print_dec(desc->height);
    display_print(" | Size: ");
    display_print_dec((uint32_t)desc->size_bytes);
    display_print(" | RefCount: ");
    display_print_dec(desc->ref_count);
    display_print(" | LockCount: ");
    display_print_dec(desc->lock_info.lock_count);
    display_print(" | State: ");
    display_print_dec((uint32_t)desc->state);
    display_print("\n");
}

void bvmm_surface_dump_all(void) {
    display_print("=== DPDP BSME SURFACE INSPECTOR DIAGNOSTICS DUMP ===\n");
    bvmm_surface_stats_t stats;
    if (bvmm_surface_get_stats(&stats) == BVMM_SUCCESS) {
        display_print("[DPDP BSME] Alive Surfaces: ");
        display_print_dec(stats.alive_surfaces);
        display_print(" | Total Created: ");
        display_print_dec(stats.total_surfaces_created);
        display_print(" | Total Memory: ");
        display_print_dec((uint32_t)stats.total_memory_bytes);
        display_print(" bytes\n");
    }
    display_print("======================================================\n");
}

bool bvmm_surface_validate_all(void) {
    bvmm_surface_stats_t stats;
    if (bvmm_surface_get_stats(&stats) != BVMM_SUCCESS) return false;
    return (stats.validation_failures == 0);
}
