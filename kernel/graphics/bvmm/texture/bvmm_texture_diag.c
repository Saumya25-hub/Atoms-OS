#include "bvmm_texture.h"
#include "kernel/drivers/display/display.h"

extern bvmm_result_t btfe_registry_lookup(btfe_texture_id_t id, btfe_texture_desc_t** out_desc);
extern uint32_t btfe_registry_get_active_count(void);

void btfe_texture_dump(btfe_texture_id_t texture_id) {
    btfe_texture_desc_t* desc = NULL;
    if (btfe_registry_lookup(texture_id, &desc) != BVMM_SUCCESS || !desc) {
        display_print("[BTFE INSPECTOR] Texture ID ");
        display_print_dec((uint32_t)texture_id);
        display_print(" NOT FOUND\n");
        return;
    }

    display_print("[BTFE INSPECTOR] Texture ID: ");
    display_print_dec((uint32_t)desc->texture_id);
    display_print(" | Res: ");
    display_print_dec(desc->width);
    display_print("x");
    display_print_dec(desc->height);
    display_print(" | Mips: ");
    display_print_dec(desc->mipchain.mip_count);
    display_print(" | Format: ");
    display_print_dec((uint32_t)desc->format);
    display_print(" | TileMode: ");
    display_print_dec((uint32_t)desc->tile_mode);
    display_print(" | TotalBytes: ");
    display_print_dec((uint32_t)desc->mipchain.total_memory_bytes);
    display_print(" | SurfaceID: ");
    display_print_dec((uint32_t)desc->surface_id);
    display_print(" | RefCount: ");
    display_print_dec(desc->ref_count);
    display_print("\n");
}

void btfe_texture_dump_all(void) {
    display_print("=== DPDP BTFE TEXTURE ENGINE DIAGNOSTICS DUMP ===\n");
    btfe_diagnostics_t diag;
    if (btfe_get_diagnostics(&diag) == BVMM_SUCCESS) {
        display_print("[DPDP BTFE] Alive Textures: ");
        display_print_dec(diag.alive_textures);
        display_print(" | Peak Textures: ");
        display_print_dec(diag.peak_textures);
        display_print(" | Total Texture Mem: ");
        display_print_dec((uint32_t)diag.total_texture_memory_bytes);
        display_print(" bytes\n");
        display_print("[DPDP BTFE] Upload Count: ");
        display_print_dec(diag.upload_count);
        display_print(" | Total Upload Bytes: ");
        display_print_dec((uint32_t)diag.upload_bytes_total);
        display_print("\n");
    }
    display_print("===================================================\n");
}

bool btfe_texture_validate_all(void) {
    btfe_diagnostics_t diag;
    if (btfe_get_diagnostics(&diag) != BVMM_SUCCESS) return false;
    return (diag.validation_failures == 0 && diag.failed_uploads == 0);
}
