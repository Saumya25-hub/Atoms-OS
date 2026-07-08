/**
 * @file agdte_backend.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Hardware Backend Abstraction
 * @status Phase 2 Core Architecture & Display Scheduler Frozen
 *
 * @section PURPOSE
 * Decouples AGDTE presentation logic from physical video drivers.
 * Manages dispatch tables for VBE, VMware SVGA, VirtIO GPU, and future hardware.
 */

#include "../include/agdte.h"

#define AGDTE_BACKEND_SLOT_COUNT 8

static AGDTE_BackendOps s_backend_table[AGDTE_BACKEND_SLOT_COUNT];
static bool s_backend_registered[AGDTE_BACKEND_SLOT_COUNT];

/* --- Native VBE Backend Bridge via BSPE --- */
static AGDTE_Error vbe_backend_init(uint32_t display_id, uint32_t width, uint32_t height, uint32_t bpp) {
    (void)display_id; (void)width; (void)height; (void)bpp;
    return AGDTE_OK;
}

static AGDTE_Error vbe_backend_set_mode(uint32_t display_id, uint32_t width, uint32_t height, uint32_t bpp) {
    (void)display_id; (void)width; (void)height; (void)bpp;
    return AGDTE_OK;
}

static AGDTE_Error vbe_backend_present_buffer(uint32_t display_id, const AGDTE_BufferDescriptor* buffer, const BOGE_Rect* dirty_rects, uint32_t dirty_count) {
    (void)display_id;
    if (!buffer || !buffer->virtual_address) {
        return AGDTE_ERR_NULL_POINTER;
    }

    BOGE_StagingFrame staging;
    staging.frame_id = buffer->buffer_id;
    staging.buffer_virtual_address = buffer->virtual_address;
    staging.pitch = buffer->pitch;
    staging.width = buffer->width;
    staging.height = buffer->height;
    staging.dirty_count = dirty_count;

    if (dirty_rects && dirty_count > 0 && dirty_count <= 32) {
        for (uint32_t i = 0; i < dirty_count; i++) {
            staging.dirty_rects[i] = dirty_rects[i];
        }
    } else {
        staging.dirty_count = 0; /* Full frame copy */
    }

    BSPE_Error bspe_err = BSPE_PresentFrame(&staging);
    if (bspe_err != BSPE_OK) {
        return AGDTE_ERR_BACKEND_FAILED;
    }

    return AGDTE_OK;
}

static AGDTE_Error vbe_backend_flip_page(uint32_t display_id, uint32_t buffer_id) {
    (void)display_id; (void)buffer_id;
    extern void vbe_swap_page(void);
    vbe_swap_page();
    return AGDTE_OK;
}

static AGDTE_Error vbe_backend_query_vsync(uint32_t display_id, bool* out_vbi_active, uint64_t* out_timestamp_us) {
    (void)display_id;
    if (out_vbi_active) *out_vbi_active = false;
    if (out_timestamp_us) *out_timestamp_us = 0;
    return AGDTE_ERR_UNSUPPORTED;
}

static AGDTE_Error vbe_backend_set_cursor_pos(uint32_t display_id, int32_t x, int32_t y) {
    (void)display_id;
    BSPE_Error err = BSPE_SetCursorPosition(x, y);
    return (err == BSPE_OK) ? AGDTE_OK : AGDTE_ERR_UNSUPPORTED;
}

static void vbe_backend_shutdown(uint32_t display_id) {
    (void)display_id;
}

static AGDTE_BackendOps s_native_vbe_ops = {
    vbe_backend_init,
    vbe_backend_set_mode,
    vbe_backend_present_buffer,
    vbe_backend_flip_page,
    vbe_backend_query_vsync,
    vbe_backend_set_cursor_pos,
    vbe_backend_shutdown
};

void agdte_backend_reset_all(void) {
    for (uint32_t i = 0; i < AGDTE_BACKEND_SLOT_COUNT; i++) {
        s_backend_table[i].init = (void*)0;
        s_backend_table[i].set_mode = (void*)0;
        s_backend_table[i].present_buffer = (void*)0;
        s_backend_table[i].flip_page = (void*)0;
        s_backend_table[i].query_vsync = (void*)0;
        s_backend_table[i].set_cursor_pos = (void*)0;
        s_backend_table[i].shutdown = (void*)0;
        s_backend_registered[i] = false;
    }

    /* Automatically register built-in VBE backend */
    s_backend_table[(uint32_t)AGDTE_BACKEND_VBE] = s_native_vbe_ops;
    s_backend_registered[(uint32_t)AGDTE_BACKEND_VBE] = true;
}

AGDTE_Error AGDTE_Backend_Register(AGDTE_BackendType type, const AGDTE_BackendOps* ops) {
    if (!ops || (uint32_t)type >= AGDTE_BACKEND_SLOT_COUNT) {
        return AGDTE_ERR_NULL_POINTER;
    }
    s_backend_table[(uint32_t)type] = *ops;
    s_backend_registered[(uint32_t)type] = true;
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Backend_SetCurrent(uint32_t display_id, AGDTE_BackendType type) {
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(display_id);
    if (!disp) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }
    if ((uint32_t)type >= AGDTE_BACKEND_SLOT_COUNT || !s_backend_registered[(uint32_t)type]) {
        return AGDTE_ERR_INVALID_STATE;
    }
    disp->backend_type = type;
    return AGDTE_OK;
}

const AGDTE_BackendOps* AGDTE_Backend_GetOps(AGDTE_BackendType type) {
    if ((uint32_t)type >= AGDTE_BACKEND_SLOT_COUNT || !s_backend_registered[(uint32_t)type]) {
        return (const AGDTE_BackendOps*)0;
    }
    return &s_backend_table[(uint32_t)type];
}
