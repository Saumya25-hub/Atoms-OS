/**
 * @file agdte_display_state.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Display State Manager
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Tracks physical and logical states of attached display controllers, refresh rates,
 * resolution geometry, cadence modes, and active front/backbuffer IDs using static pools.
 */

#include "../include/agdte.h"

/* --- Static Storage for Display Controllers --- */
static AGDTE_DisplayState s_displays[AGDTE_MAX_DISPLAYS];
static uint32_t s_registered_display_count = 0;

void agdte_display_reset_all(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_displays[i].display_id = i;
        s_displays[i].active = false;
        s_displays[i].width = 0;
        s_displays[i].height = 0;
        s_displays[i].bpp = 0;
        s_displays[i].pitch_bytes = 0;
        s_displays[i].backend_type = AGDTE_BACKEND_NONE;
        s_displays[i].cadence_mode = AGDTE_CADENCE_60HZ_FIXED;
        s_displays[i].refresh_rate_hz = 60;
        s_displays[i].last_vbi_timestamp_us = 0;
        s_displays[i].active_frontbuffer_id = 0;
        s_displays[i].active_backbuffer_id = 0;
    }
    s_registered_display_count = 0;
}

AGDTE_Error AGDTE_Display_Register(uint32_t width, uint32_t height, uint32_t bpp, AGDTE_BackendType backend, uint32_t* out_display_id) {
    if (!out_display_id) {
        return AGDTE_ERR_NULL_POINTER;
    }
    if (width == 0 || height == 0 || bpp == 0) {
        return AGDTE_ERR_INVALID_STATE;
    }
    if (s_registered_display_count >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_OUT_OF_MEMORY;
    }

    /* Find first inactive display slot */
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        if (!s_displays[i].active) {
            s_displays[i].display_id = i;
            s_displays[i].active = true;
            s_displays[i].width = width;
            s_displays[i].height = height;
            s_displays[i].bpp = bpp;
            s_displays[i].pitch_bytes = width * (bpp / 8);
            s_displays[i].backend_type = backend;
            s_displays[i].cadence_mode = AGDTE_CADENCE_60HZ_FIXED;
            s_displays[i].refresh_rate_hz = 60;
            s_displays[i].last_vbi_timestamp_us = 0;
            s_displays[i].active_frontbuffer_id = 0;
            s_displays[i].active_backbuffer_id = 0;

            *out_display_id = i;
            s_registered_display_count++;

            /* Notify hardware abstraction backend if available */
            const AGDTE_BackendOps* ops = AGDTE_Backend_GetOps(backend);
            if (ops && ops->init) {
                ops->init(i, width, height, bpp);
            }

            return AGDTE_OK;
        }
    }

    return AGDTE_ERR_OUT_OF_MEMORY;
}

AGDTE_Error AGDTE_Display_SetCadenceMode(uint32_t display_id, AGDTE_CadenceMode mode, uint32_t refresh_hz) {
    if (display_id >= AGDTE_MAX_DISPLAYS || !s_displays[display_id].active) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    s_displays[display_id].cadence_mode = mode;
    if (refresh_hz > 0) {
        s_displays[display_id].refresh_rate_hz = refresh_hz;
    } else {
        /* Default to reasonable refresh rates per cadence mode */
        switch (mode) {
            case AGDTE_CADENCE_144HZ_FIXED:
                s_displays[display_id].refresh_rate_hz = 144;
                break;
            case AGDTE_CADENCE_IMMEDIATE:
            case AGDTE_CADENCE_60HZ_FIXED:
            case AGDTE_CADENCE_VSYNC_IRQ:
            case AGDTE_CADENCE_ADAPTIVE_SYNC:
            default:
                s_displays[display_id].refresh_rate_hz = 60;
                break;
        }
    }

    return AGDTE_OK;
}

AGDTE_DisplayState* AGDTE_Display_GetState(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS || !s_displays[display_id].active) {
        return (AGDTE_DisplayState*)0;
    }
    return &s_displays[display_id];
}
