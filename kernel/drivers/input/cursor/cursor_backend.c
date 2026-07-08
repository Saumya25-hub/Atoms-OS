/**
 * @file cursor_backend.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Backend Implementation
 * @section PURPOSE
 * Manages BSPE Hardware Cursor Plane connection and software fallback mode switching.
 */

#include "cursor_backend.h"
#include "cursor_state.h"
#include "cursor_diag.h"
#include "kernel/graphics/BSPE/Cursor/cursor_plane.h"
#include <stddef.h>

static BSPE_CursorPlaneHandle g_hw_handle = 0;
static CursorBackendType g_active_backend = CURSOR_BACKEND_NONE;

void cursor_backend_init(void) {
    BSPE_CursorPlaneConfig config;
    config.max_width = 64;
    config.max_height = 64;
    config.allow_software_fallback = true;
    config.force_software_mode = false;

    BSPE_Error err = BSPE_CursorPlane_Create(&config, &g_hw_handle);
    if (err == BSPE_OK && g_hw_handle != 0 && BSPE_CursorPlane_IsHardwareSupported(g_hw_handle)) {
        g_active_backend = CURSOR_BACKEND_HARDWARE;
        cursor_state_set_backend_mode(true, false);
        cursor_diag_set_backend(1);
    } else {
        /* Fallback to software cursor rendering */
        g_active_backend = CURSOR_BACKEND_SOFTWARE;
        cursor_state_set_backend_mode(false, true);
        cursor_diag_set_backend(2);
        cursor_diag_log_fallback();
    }
}

void cursor_backend_shutdown(void) {
    if (g_hw_handle != 0) {
        BSPE_CursorPlane_Destroy(g_hw_handle);
        g_hw_handle = 0;
    }
    g_active_backend = CURSOR_BACKEND_NONE;
}

CursorBackendType cursor_backend_get_type(void) {
    return g_active_backend;
}

bool cursor_backend_is_hardware(void) {
    return (g_active_backend == CURSOR_BACKEND_HARDWARE);
}

void cursor_backend_set_position(int32_t x, int32_t y) {
    if (g_active_backend == CURSOR_BACKEND_HARDWARE && g_hw_handle != 0) {
        BSPE_Error err = BSPE_CursorPlane_SetPosition(g_hw_handle, x, y);
        if (err != BSPE_OK) {
            /* Seamlesly switch to software fallback on hardware error */
            g_active_backend = CURSOR_BACKEND_SOFTWARE;
            cursor_state_set_backend_mode(false, true);
            cursor_diag_set_backend(2);
            cursor_diag_log_fallback();
        }
    }
}

void cursor_backend_set_image(const uint32_t* argb_bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y) {
    if (g_active_backend == CURSOR_BACKEND_HARDWARE && g_hw_handle != 0) {
        BSPE_Error err = BSPE_CursorPlane_SetImage(g_hw_handle, argb_bitmap, width, height, hotspot_x, hotspot_y);
        if (err != BSPE_OK) {
            g_active_backend = CURSOR_BACKEND_SOFTWARE;
            cursor_state_set_backend_mode(false, true);
            cursor_diag_set_backend(2);
            cursor_diag_log_fallback();
        }
    }
}

void cursor_backend_set_visibility(bool visible) {
    if (g_active_backend == CURSOR_BACKEND_HARDWARE && g_hw_handle != 0) {
        BSPE_Error err = BSPE_CursorPlane_SetVisibility(g_hw_handle, visible);
        if (err != BSPE_OK) {
            g_active_backend = CURSOR_BACKEND_SOFTWARE;
            cursor_state_set_backend_mode(false, true);
            cursor_diag_set_backend(2);
            cursor_diag_log_fallback();
        }
    }
}
