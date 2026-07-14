/**
 * @file cursor_plane.c
 * @brief BSPE Cursor Plane Production Implementation
 * @status Step 9 Production Implementation
 * 
 * @section PURPOSE
 * Implements an abstracted hardware and software cursor plane without rendering, compositor changes,
 * or VRAM copies. Enforces strict separation between mouse movement and scene redraws.
 */

#include "cursor_plane.h"
#include <stddef.h>
#include "kernel/debug/step14_telemetry.h"

#define BSPE_CP_MAX_INSTANCES 2
#define BSPE_CP_MAX_SPRITE_DIM 64

/* --- Driver Abstraction Interface --- */
typedef struct {
    BSPE_Error (*set_pos)(void* driver_ctx, int32_t sprite_x, int32_t sprite_y);
    BSPE_Error (*set_image)(void* driver_ctx, const uint32_t* bitmap, uint32_t w, uint32_t h);
    BSPE_Error (*set_visible)(void* driver_ctx, bool visible);
} BSPE_CursorDriverOps;

typedef struct BSPE_CursorPlane_T {
    BSPE_CursorMode mode;
    bool visible;
    int32_t screen_x;
    int32_t screen_y;
    int32_t sprite_x;
    int32_t sprite_y;
    
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    
    /* Fixed sprite memory (No heap allocation) */
    uint32_t sprite_bitmap[BSPE_CP_MAX_SPRITE_DIM * BSPE_CP_MAX_SPRITE_DIM];
    
    /* Driver abstraction */
    BSPE_CursorDriverOps hw_ops;
    BSPE_CursorDriverOps sw_ops;
    void* hw_ctx;
    void* sw_ctx;
    
    bool allow_software_fallback;
    bool force_software_mode;
    bool is_allocated;
    
    /* Telemetry */
    uint32_t pos_update_count;
    uint32_t img_update_count;
    uint32_t fallback_count;
} BSPE_CursorPlaneInstance;

/* Static pool in kernel BSS segment (No heap allocation) */
static BSPE_CursorPlaneInstance g_cp_pool[BSPE_CP_MAX_INSTANCES];

/* --- Simulated Hardware Driver (Bochs / VBE / GPU registers) --- */
static BSPE_Error hw_driver_set_pos(void* ctx, int32_t sx, int32_t sy) {
    (void)ctx; (void)sx; (void)sy;
    g_step14_telemetry.hw_cursor_updates++;
    /* Engineering Safety Rule: Bochs VBE does not support hardware cursor. Must return UNSUPPORTED */
    return BSPE_ERR_UNSUPPORTED; 
}
static BSPE_Error hw_driver_set_image(void* ctx, const uint32_t* bmp, uint32_t w, uint32_t h) {
    (void)ctx; (void)bmp; (void)w; (void)h;
    return BSPE_OK;
}
static BSPE_Error hw_driver_set_visible(void* ctx, bool vis) {
    (void)ctx; (void)vis;
    return BSPE_OK;
}

/* --- Simulated Software Fallback Driver --- */
static BSPE_Error sw_driver_set_pos(void* ctx, int32_t sx, int32_t sy) {
    (void)ctx; (void)sx; (void)sy;
    g_step14_telemetry.sw_cursor_draws++;
    /* Software sprite position recorded; zero rendering or VRAM copies in Step 9 */
    return BSPE_OK;
}
static BSPE_Error sw_driver_set_image(void* ctx, const uint32_t* bmp, uint32_t w, uint32_t h) {
    (void)ctx; (void)bmp; (void)w; (void)h;
    return BSPE_OK;
}
static BSPE_Error sw_driver_set_visible(void* ctx, bool vis) {
    (void)ctx; (void)vis;
    return BSPE_OK;
}

/* --- Simulated Failing Hardware Driver (for Fallback Testing) --- */
static BSPE_Error hw_driver_fail_pos(void* ctx, int32_t sx, int32_t sy) {
    (void)ctx; (void)sx; (void)sy;
    return BSPE_ERR_UNSUPPORTED; /* Simulate hardware register failure */
}

/* --- Lifecycle Implementations --- */

BSPE_Error BSPE_CursorPlane_Create(const BSPE_CursorPlaneConfig* config, BSPE_CursorPlaneHandle* out_handle) {
    if (!out_handle) return BSPE_ERR_NULL_POINTER;
    
    for (uint32_t i = 0; i < BSPE_CP_MAX_INSTANCES; i++) {
        if (!g_cp_pool[i].is_allocated) {
            g_cp_pool[i].is_allocated = true;
            g_cp_pool[i].allow_software_fallback = config ? config->allow_software_fallback : true;
            g_cp_pool[i].force_software_mode     = config ? config->force_software_mode : false;
            
            g_cp_pool[i].visible   = false;
            g_cp_pool[i].screen_x  = 0;
            g_cp_pool[i].screen_y  = 0;
            g_cp_pool[i].sprite_x  = 0;
            g_cp_pool[i].sprite_y  = 0;
            g_cp_pool[i].width     = 0;
            g_cp_pool[i].height    = 0;
            g_cp_pool[i].hotspot_x = 0;
            g_cp_pool[i].hotspot_y = 0;
            g_cp_pool[i].pos_update_count = 0;
            g_cp_pool[i].img_update_count = 0;
            g_cp_pool[i].fallback_count   = 0;
            
            /* Bind driver vtables */
            g_cp_pool[i].hw_ops.set_pos     = hw_driver_set_pos;
            g_cp_pool[i].hw_ops.set_image   = hw_driver_set_image;
            g_cp_pool[i].hw_ops.set_visible = hw_driver_set_visible;
            g_cp_pool[i].hw_ctx             = &g_cp_pool[i];
            
            g_cp_pool[i].sw_ops.set_pos     = sw_driver_set_pos;
            g_cp_pool[i].sw_ops.set_image   = sw_driver_set_image;
            g_cp_pool[i].sw_ops.set_visible = sw_driver_set_visible;
            g_cp_pool[i].sw_ctx             = &g_cp_pool[i];
            
            if (g_cp_pool[i].force_software_mode) {
                g_cp_pool[i].mode = BSPE_CURSOR_MODE_SOFTWARE;
            } else {
                g_cp_pool[i].mode = BSPE_CURSOR_MODE_HARDWARE;
            }
            
            *out_handle = (BSPE_CursorPlaneHandle)&g_cp_pool[i];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_CursorPlane_Destroy(BSPE_CursorPlaneHandle handle) {
    if (!handle) return;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    cp->is_allocated = false;
}

BSPE_Error BSPE_CursorPlane_SetMode(BSPE_CursorPlaneHandle handle, BSPE_CursorMode mode) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    cp->mode = mode;
    if (mode == BSPE_CURSOR_MODE_SOFTWARE) {
        BSPE_CursorPlane_SyncSoftwareFallback();
    }
    return BSPE_OK;
}

/* --- Core Cursor Operations --- */

BSPE_Error BSPE_CursorPlane_SetPosition(BSPE_CursorPlaneHandle handle, int32_t x, int32_t y) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    
    cp->screen_x = x;
    cp->screen_y = y;
    cp->sprite_x = x - (int32_t)cp->hotspot_x;
    cp->sprite_y = y - (int32_t)cp->hotspot_y;
    cp->pos_update_count++;
    
    BSPE_Error err = BSPE_OK;
    if (cp->mode == BSPE_CURSOR_MODE_HARDWARE) {
        err = cp->hw_ops.set_pos(cp->hw_ctx, cp->sprite_x, cp->sprite_y);
        if (err != BSPE_OK) {
            if (cp->allow_software_fallback) {
                cp->mode = BSPE_CURSOR_MODE_SOFTWARE;
                cp->fallback_count++;
                BSPE_CursorPlane_SyncSoftwareFallback();
                err = cp->sw_ops.set_pos(cp->sw_ctx, cp->sprite_x, cp->sprite_y);
            }
        }
    } else if (cp->mode == BSPE_CURSOR_MODE_SOFTWARE) {
        err = cp->sw_ops.set_pos(cp->sw_ctx, cp->sprite_x, cp->sprite_y);
    }
    
    return err;
}

BSPE_Error BSPE_CursorPlane_SetImage(BSPE_CursorPlaneHandle handle, const uint32_t* argb_bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    if (!argb_bitmap && (width > 0 || height > 0)) return BSPE_ERR_NULL_POINTER;
    if (width > BSPE_CP_MAX_SPRITE_DIM || height > BSPE_CP_MAX_SPRITE_DIM) return BSPE_ERR_UNSUPPORTED;
    if (width > 0 && hotspot_x >= width) return BSPE_ERR_INVALID_STATE;
    if (height > 0 && hotspot_y >= height) return BSPE_ERR_INVALID_STATE;
    
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    cp->width = width;
    cp->height = height;
    cp->hotspot_x = hotspot_x;
    cp->hotspot_y = hotspot_y;
    cp->img_update_count++;
    
    /* Copy bitmap to internal static sprite RAM (Zero heap allocation, zero VRAM copy) */
    uint32_t count = width * height;
    for (uint32_t i = 0; i < count; i++) {
        cp->sprite_bitmap[i] = argb_bitmap ? argb_bitmap[i] : 0;
    }
    
    /* Recalculate sprite coordinate with new hotspot */
    cp->sprite_x = cp->screen_x - (int32_t)cp->hotspot_x;
    cp->sprite_y = cp->screen_y - (int32_t)cp->hotspot_y;
    
    BSPE_Error err = BSPE_OK;
    if (cp->mode == BSPE_CURSOR_MODE_HARDWARE) {
        err = cp->hw_ops.set_image(cp->hw_ctx, cp->sprite_bitmap, width, height);
        if (err != BSPE_OK && cp->allow_software_fallback) {
            cp->mode = BSPE_CURSOR_MODE_SOFTWARE;
            cp->fallback_count++;
            BSPE_CursorPlane_SyncSoftwareFallback();
            err = cp->sw_ops.set_image(cp->sw_ctx, cp->sprite_bitmap, width, height);
        }
    } else if (cp->mode == BSPE_CURSOR_MODE_SOFTWARE) {
        err = cp->sw_ops.set_image(cp->sw_ctx, cp->sprite_bitmap, width, height);
    }
    
    return err;
}

BSPE_Error BSPE_CursorPlane_SetVisibility(BSPE_CursorPlaneHandle handle, bool visible) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    
    cp->visible = visible;
    BSPE_Error err = BSPE_OK;
    if (cp->mode == BSPE_CURSOR_MODE_HARDWARE) {
        err = cp->hw_ops.set_visible(cp->hw_ctx, visible);
        if (err != BSPE_OK && cp->allow_software_fallback) {
            cp->mode = BSPE_CURSOR_MODE_SOFTWARE;
            cp->fallback_count++;
            BSPE_CursorPlane_SyncSoftwareFallback();
            err = cp->sw_ops.set_visible(cp->sw_ctx, visible);
        }
    } else if (cp->mode == BSPE_CURSOR_MODE_SOFTWARE) {
        err = cp->sw_ops.set_visible(cp->sw_ctx, visible);
    }
    return err;
}

BSPE_Error BSPE_CursorPlane_Show(BSPE_CursorPlaneHandle handle) {
    return BSPE_CursorPlane_SetVisibility(handle, true);
}

BSPE_Error BSPE_CursorPlane_Hide(BSPE_CursorPlaneHandle handle) {
    return BSPE_CursorPlane_SetVisibility(handle, false);
}

BSPE_Error BSPE_CursorPlane_GetPosition(BSPE_CursorPlaneHandle handle, int32_t* out_x, int32_t* out_y) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    if (out_x) *out_x = cp->screen_x;
    if (out_y) *out_y = cp->screen_y;
    return BSPE_OK;
}

BSPE_Error BSPE_CursorPlane_GetState(BSPE_CursorPlaneHandle handle, bool* out_visible, BSPE_CursorMode* out_mode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_hotspot_x, uint32_t* out_hotspot_y) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    if (out_visible)   *out_visible   = cp->visible;
    if (out_mode)      *out_mode      = cp->mode;
    if (out_width)     *out_width     = cp->width;
    if (out_height)    *out_height    = cp->height;
    if (out_hotspot_x) *out_hotspot_x = cp->hotspot_x;
    if (out_hotspot_y) *out_hotspot_y = cp->hotspot_y;
    return BSPE_OK;
}

bool BSPE_CursorPlane_IsHardwareSupported(BSPE_CursorPlaneHandle handle) {
    if (!handle) return false;
    BSPE_CursorPlaneInstance* cp = (BSPE_CursorPlaneInstance*)handle;
    return (cp->mode == BSPE_CURSOR_MODE_HARDWARE);
}

void BSPE_CursorPlane_SyncSoftwareFallback(void) {
    for (uint32_t i = 0; i < BSPE_CP_MAX_INSTANCES; i++) {
        if (g_cp_pool[i].is_allocated && g_cp_pool[i].mode == BSPE_CURSOR_MODE_HARDWARE) {
            g_cp_pool[i].mode = BSPE_CURSOR_MODE_SOFTWARE;
        }
    }
}

/* --- Self-Test Verification Suite --- */

bool BSPE_CursorPlane_RunSelfTest(void) {
    BSPE_CursorPlaneHandle cp = NULL;
    BSPE_CursorPlaneConfig cfg = { .max_width = 64, .max_height = 64, .allow_software_fallback = true };
    
    /* 1. Position Update Test */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    if (BSPE_CursorPlane_SetPosition(cp, 500, 300) != BSPE_OK) return false;
    int32_t x = 0, y = 0;
    if (BSPE_CursorPlane_GetPosition(cp, &x, &y) != BSPE_OK || x != 500 || y != 300) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    /* 2. Visibility Test */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    bool vis = true;
    BSPE_CursorPlane_GetState(cp, &vis, NULL, NULL, NULL, NULL, NULL);
    if (vis != false) return false; /* Starts hidden */
    if (BSPE_CursorPlane_Show(cp) != BSPE_OK) return false;
    BSPE_CursorPlane_GetState(cp, &vis, NULL, NULL, NULL, NULL, NULL);
    if (vis != true) return false;
    if (BSPE_CursorPlane_Hide(cp) != BSPE_OK) return false;
    BSPE_CursorPlane_GetState(cp, &vis, NULL, NULL, NULL, NULL, NULL);
    if (vis != false) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    /* 3. Hotspot Test */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    uint32_t dummy_bmp[32 * 32];
    if (BSPE_CursorPlane_SetImage(cp, dummy_bmp, 32, 32, 10, 15) != BSPE_OK) return false;
    if (BSPE_CursorPlane_SetPosition(cp, 100, 100) != BSPE_OK) return false;
    BSPE_CursorPlaneInstance* inst = (BSPE_CursorPlaneInstance*)cp;
    /* Verify sprite coordinate adjusted by hotspot: (100 - 10, 100 - 15) = (90, 85) */
    if (inst->sprite_x != 90 || inst->sprite_y != 85) return false;
    /* Verify illegal hotspot (e.g. hotspot_x >= width) is rejected */
    if (BSPE_CursorPlane_SetImage(cp, dummy_bmp, 32, 32, 40, 10) != BSPE_ERR_INVALID_STATE) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    /* 4. Image Replacement Test (32x32 -> 64x64) */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    uint32_t big_bmp[64 * 64];
    if (BSPE_CursorPlane_SetImage(cp, dummy_bmp, 32, 32, 0, 0) != BSPE_OK) return false;
    uint32_t w = 0, h = 0;
    BSPE_CursorPlane_GetState(cp, NULL, NULL, &w, &h, NULL, NULL);
    if (w != 32 || h != 32) return false;
    if (BSPE_CursorPlane_SetImage(cp, big_bmp, 64, 64, 31, 31) != BSPE_OK) return false;
    BSPE_CursorPlane_GetState(cp, NULL, NULL, &w, &h, NULL, NULL);
    if (w != 64 || h != 64) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    /* 5. Driver Fallback Test */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    inst = (BSPE_CursorPlaneInstance*)cp;
    if (inst->mode != BSPE_CURSOR_MODE_HARDWARE) return false;
    /* Inject failing hardware driver operation */
    inst->hw_ops.set_pos = hw_driver_fail_pos;
    /* When we call SetPosition, HW fails -> Must seamlessly fall back to SOFTWARE mode! */
    if (BSPE_CursorPlane_SetPosition(cp, 200, 200) != BSPE_OK) return false;
    if (inst->mode != BSPE_CURSOR_MODE_SOFTWARE || inst->fallback_count != 1) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    /* 6. State Validation & Stress Test (1,000 rapid updates) */
    if (BSPE_CursorPlane_Create(&cfg, &cp) != BSPE_OK || !cp) return false;
    for (int i = 0; i < 1000; i++) {
        BSPE_CursorPlane_SetPosition(cp, i, i * 2);
        BSPE_CursorPlane_SetVisibility(cp, (i % 2 == 0));
    }
    inst = (BSPE_CursorPlaneInstance*)cp;
    if (inst->pos_update_count != 1000) return false;
    BSPE_CursorPlane_Destroy(cp);
    
    return true;
}

#ifdef BSPE_TEST_HARNESS
#include <stdio.h>
int main(void) {
    printf("[BSPE Test] Running Cursor Plane Self-Test Suite...\n");
    if (BSPE_CursorPlane_RunSelfTest()) {
        printf("[BSPE Test] ALL TESTS PASSED: Position, Visibility, Hotspot, Image Replace, Driver Fallback, State Validation!\n");
        return 0;
    } else {
        printf("[BSPE Test] SELF-TEST FAILED!\n");
        return 1;
    }
}
#endif
