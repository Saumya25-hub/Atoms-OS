/**
 * @file cursor_renderer.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Renderer Implementation
 * @section PURPOSE
 * Implements deterministic rendering and dirty region optimization without heap allocation.
 */

#include "cursor_renderer.h"
#include "cursor_state.h"
#include "cursor_theme.h"
#include "cursor_animation.h"
#include "cursor_backend.h"
#include "cursor_diag.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/graphics.h"
#include "kernel/debug/step14_telemetry.h"
#include <stddef.h>

/* Static reference for dirty box query */
static CursorBoundingBox g_last_dirty_union;

extern void BSPE_CursorPresenter_Init(void);
extern void BSPE_CursorPresenter_UpdatePosition(int32_t screen_x, int32_t screen_y, const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y, bool visible, uint32_t scale_percent);
extern void BSPE_CursorPresenter_OnCompositorRedraw(const BVFramebuffer* ram_fb, const BVFramebuffer* hw_fb);
extern void BSPE_CursorPresenter_RestoreBackground(const BVFramebuffer* target_fb);

void cursor_renderer_init(void) {
    g_last_dirty_union.is_valid = false;
    BSPE_CursorPresenter_Init();
}

void cursor_renderer_update(bool moved) {
    /* 1. Check animation tick */
    bool anim_changed = cursor_animation_tick();
    if (anim_changed) {
        cursor_diag_log_anim_frame();
    }

    /* 2. Asynchronous Cursor Presentation (Phase 2 - Graphics Presentation Engine V2) */
    CursorState state;
    cursor_state_get_snapshot(&state);
    
    uint32_t w = 32, h = 32, hx = 0, hy = 0;
    const uint32_t* bmp = cursor_theme_get_bitmap(state.current_shape, state.current_anim_frame, &w, &h, &hx, &hy);
    
    BSPE_CursorPresenter_UpdatePosition(state.screen_x, state.screen_y, bmp, w, h, hx, hy, state.visible, state.scale_percent);
}

void cursor_renderer_restore_background(const void* fb_ptr) {
    BSPE_CursorPresenter_RestoreBackground((const BVFramebuffer*)fb_ptr);
}

void cursor_renderer_draw_software(const void* fb_ptr) {
    BSPE_CursorPresenter_OnCompositorRedraw((const BVFramebuffer*)fb_ptr, 0);
}

void cursor_renderer_get_dirty_box(CursorBoundingBox* out_box) {
    if (!out_box) return;
    *out_box = g_last_dirty_union;
}
