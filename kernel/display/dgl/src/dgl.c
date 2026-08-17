#include "kernel/display/dgl/include/dgl.h"

/*
 * ⚛️ DGL — DISPLAY GOVERNANCE LAYER V1.0 IMPLEMENTATION
 */

static dgl_geometry_t      g_dgl_geom;
static dgl_display_state_t g_dgl_state = DGL_STATE_BOOT;

void dgl_init(uint32_t phys_w, uint32_t phys_h, uint32_t pitch_bytes) {
    g_dgl_geom.phys_width = (phys_w > 0) ? phys_w : 1024;
    g_dgl_geom.phys_height = (phys_h > 0) ? phys_h : 768;
    g_dgl_geom.pitch_bytes = (pitch_bytes > 0) ? pitch_bytes : (g_dgl_geom.phys_width * 4);
    g_dgl_geom.stride_pixels = g_dgl_geom.pitch_bytes / 4;
    g_dgl_geom.bytes_per_pixel = 4;
    g_dgl_geom.quiet_boot_enabled = true; /* Quiet Boot Active By Default */

    g_dgl_state = DGL_STATE_BOOT;
    bram_request_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
}

int dgl_set_state(dgl_display_state_t state) {
    g_dgl_state = state;
    switch (state) {
        case DGL_STATE_BOOT:
        case DGL_STATE_LOGIN:
            return bram_request_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
        case DGL_STATE_DESKTOP:
            g_dgl_geom.quiet_boot_enabled = false;
            return bram_request_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_BOSURFACE_COMPOSITOR);
        case DGL_STATE_RECOVERY:
            return bram_request_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ABDE_DIAGNOSTICS);
        default:
            return DGL_ERR_INVALID_PARAM;
    }
}

dgl_display_state_t dgl_get_state(void) {
    return g_dgl_state;
}

const dgl_geometry_t* dgl_get_geometry(void) {
    return &g_dgl_geom;
}

bool dgl_can_draw(bram_module_id_t module_id) {
    /* If quiet boot is enabled or state is BOOT, ONLY Rook Engine / Kernel Core can present */
    if (g_dgl_geom.quiet_boot_enabled || g_dgl_state == DGL_STATE_BOOT) {
        if (module_id != BRAM_MODULE_ROOK_ENGINE && module_id != BRAM_MODULE_KERNEL_CORE) {
            return false;
        }
    }
    return bram_has_ownership(BRAM_RESOURCE_DISPLAY, module_id);
}

void dgl_set_quiet_boot(bool quiet) {
    g_dgl_geom.quiet_boot_enabled = quiet;
}

bool dgl_is_quiet_boot(void) {
    return g_dgl_geom.quiet_boot_enabled;
}
