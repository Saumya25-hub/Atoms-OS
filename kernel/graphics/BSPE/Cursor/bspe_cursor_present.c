/**
 * @file bspe_cursor_present.c
 * @brief BSPE Asynchronous Cursor Presentation Layer (Phase 2)
 * @section PURPOSE
 * Implements asynchronous cursor presentation to physical VRAM and system RAM.
 * Achieves 1000 Hz visual continuity without waiting for 60 Hz compositor frame clock.
 */

#include "bspe_cursor_present.h"
#include <stddef.h>
#include "kernel/drivers/input/cursor/cursor_hotspot.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/drivers/input/cursor/cursor_backend.h"
#include "kernel/drivers/input/cursor/cursor_diag.h"
#include "../include/bspe.h"
#include "../../BOGE/include/boge.h"
#include "kernel/debug/step14_telemetry.h"
#include "kernel/drivers/display/display.h"

extern void* BOVISUAL_Graphics_GetBuffer(void);
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern BVFramebuffer* vbe_get_framebuffer(void);
extern BVFramebuffer vbe_get_back_page(void);
extern void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);

static BSPE_CursorPresenterState s_state;
static uint32_t s_bitmap[64 * 64];
static CursorBoundingBox s_prev_box;
static CursorBoundingBox s_last_union;

/* Phase 3 Fast Path State (Disabled to enforce single-writer compositor overlay and eliminate cursor trail artifacts) */
bool g_bspe_cursor_fast_path_enabled = false;
static uint32_t s_shadow_buffer[64 * 64];
static CursorBoundingBox s_shadow_box;
static bool s_shadow_valid = false;
static bool s_compositor_owns_buffer = false;
static bool s_cursor_pending = false;
static int32_t s_requested_x = 320;
static int32_t s_requested_y = 240;

extern volatile uint64_t g_cursor_position_requests;
extern volatile uint64_t g_cursor_fast_presents;
extern volatile uint64_t g_cursor_updates_coalesced;
extern volatile uint64_t g_cursor_fast_path_max_us;
extern volatile uint64_t g_cursor_fast_path_total_us;
extern volatile uint64_t g_cursor_blocked_by_compositor;
extern volatile uint64_t g_cursor_fallbacks;

volatile uint64_t g_cursor_pump_calls = 0;
volatile uint64_t g_cursor_pump_pending_consumed = 0;
volatile uint64_t g_cursor_pump_no_pending = 0;
volatile uint64_t g_cursor_pending_age_max_us = 0;
volatile uint64_t g_cursor_pending_over_2ms = 0;
volatile uint64_t g_cursor_pending_over_5ms = 0;
volatile uint64_t g_cursor_pending_over_16ms = 0;
volatile uint64_t g_cursor_pending_over_50ms = 0;

static uint64_t s_pending_start_tsc = 0;

static void cp_restore_shadow(const BVFramebuffer* fb) {
    if (!s_shadow_valid || !s_shadow_box.is_valid || !fb || !fb->buffer) return;
    uint32_t pitch_pixels = fb->pitch / 4;
    if (pitch_pixels == 0) pitch_pixels = fb->width;

    for (uint32_t y = 0; y < s_shadow_box.draw_h; y++) {
        for (uint32_t x = 0; x < s_shadow_box.draw_w; x++) {
            uint32_t screen_idx = (s_shadow_box.draw_y + y) * pitch_pixels + (s_shadow_box.draw_x + x);
            if ((s_shadow_box.draw_y + y) < fb->height && (s_shadow_box.draw_x + x) < fb->width) {
                fb->buffer[screen_idx] = s_shadow_buffer[y * s_shadow_box.draw_w + x];
            }
        }
    }
    s_shadow_valid = false;
}

static void cp_capture_shadow(const CursorBoundingBox* box, const BVFramebuffer* fb) {
    if (!box || !box->is_valid || !fb || !fb->buffer) return;
    uint32_t pitch_pixels = fb->pitch / 4;
    if (pitch_pixels == 0) pitch_pixels = fb->width;

    s_shadow_box = *box;
    for (uint32_t y = 0; y < box->draw_h; y++) {
        for (uint32_t x = 0; x < box->draw_w; x++) {
            uint32_t screen_idx = (box->draw_y + y) * pitch_pixels + (box->draw_x + x);
            if ((box->draw_y + y) < fb->height && (box->draw_x + x) < fb->width) {
                s_shadow_buffer[y * box->draw_w + x] = fb->buffer[screen_idx];
            }
        }
    }
    s_shadow_valid = true;
}

static void cp_draw_box(const CursorBoundingBox* box, const BVFramebuffer* fb, const uint32_t* bmp, uint32_t w, uint32_t h, uint32_t scale_percent) {
    if (!box || !box->is_valid || !fb || !fb->buffer || fb->width == 0 || fb->height == 0 || !bmp) return;
    uint32_t pitch_pixels = fb->pitch / 4;
    if (pitch_pixels == 0) pitch_pixels = fb->width;

    for (uint32_t y = 0; y < box->draw_h; y++) {
        uint32_t sprite_y = box->sprite_offset_y + (y * 100) / scale_percent;
        if (sprite_y >= h) sprite_y = h - 1;

        for (uint32_t x = 0; x < box->draw_w; x++) {
            uint32_t sprite_x = box->sprite_offset_x + (x * 100) / scale_percent;
            if (sprite_x >= w) sprite_x = w - 1;

            uint32_t argb = bmp[sprite_y * w + sprite_x];
            uint32_t alpha = (argb >> 24) & 0xFF;
            if (alpha == 0) continue;

            uint32_t screen_idx = (box->draw_y + y) * pitch_pixels + (box->draw_x + x);
            if ((box->draw_y + y) >= fb->height || (box->draw_x + x) >= fb->width) continue;

            if (alpha == 255) {
                fb->buffer[screen_idx] = argb;
            } else {
                uint32_t bg = fb->buffer[screen_idx];
                uint32_t inv_alpha = 255 - alpha;
                uint32_t r = (((argb >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha) / 255;
                uint32_t g = (((argb >> 8) & 0xFF) * alpha + ((bg >> 8) & 0xFF) * inv_alpha) / 255;
                uint32_t b = ((argb & 0xFF) * alpha + (bg & 0xFF) * inv_alpha) / 255;
                fb->buffer[screen_idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

void BSPE_CursorPresenter_Init(void) {
    s_state.is_initialized = true;
    s_state.visible = true;
    s_state.scale_percent = 100;
    s_state.width = 32;
    s_state.height = 32;
    s_state.hotspot_x = 0;
    s_state.hotspot_y = 0;
    s_state.current_x = 320;
    s_state.current_y = 240;
    s_prev_box.is_valid = false;
    s_last_union.is_valid = false;

    extern const uint32_t* cursor_theme_get_bitmap(uint32_t shape, uint32_t frame, uint32_t* w, uint32_t* h, uint32_t* hx, uint32_t* hy);
    uint32_t w = 32, h = 32, hx = 0, hy = 0;
    const uint32_t* bmp = cursor_theme_get_bitmap(0 /* ARROW */, 0, &w, &h, &hx, &hy);
    if (bmp && w <= 64 && h <= 64) {
        for (uint32_t i = 0; i < w * h; i++) {
            s_bitmap[i] = bmp[i];
        }
        s_state.width = w;
        s_state.height = h;
        s_state.hotspot_x = hx;
        s_state.hotspot_y = hy;
    }
}

void BSPE_CursorPresenter_SetCoords(int32_t x, int32_t y) {
    s_state.current_x = x;
    s_state.current_y = y;
    s_state.visible = true;
}

void BSPE_CursorPresenter_UpdateBitmap(const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y) {
    if (!bitmap || width > 64 || height > 64) return;
    s_state.width = width;
    s_state.height = height;
    s_state.hotspot_x = hotspot_x;
    s_state.hotspot_y = hotspot_y;
    for (uint32_t i = 0; i < width * height; i++) {
        s_bitmap[i] = bitmap[i];
    }
}

void BSPE_CursorPresenter_UpdatePosition(int32_t screen_x, int32_t screen_y, const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y, bool visible, uint32_t scale_percent) {
    uint64_t start_tsc = step14_rdtsc();
    
    g_cursor_position_requests++;
    
    /* Update fast path requested state */
    s_requested_x = screen_x;
    s_requested_y = screen_y;
    if (!s_cursor_pending) {
        s_cursor_pending = true;
        s_pending_start_tsc = start_tsc;
    }

    /* Maintain fallback V3 State */
    s_state.current_x = screen_x;
    s_state.current_y = screen_y;
    s_state.width = width;
    s_state.height = height;
    s_state.hotspot_x = hotspot_x;
    s_state.hotspot_y = hotspot_y;
    s_state.visible = visible;
    s_state.scale_percent = (scale_percent > 0) ? scale_percent : 100;
    
    if (bitmap && width <= 64 && height <= 64) {
        for (uint32_t i = 0; i < width * height; i++) {
            s_bitmap[i] = bitmap[i];
        }
    }
    
    if (cursor_backend_is_hardware()) {
        if (!visible) {
            cursor_backend_set_visibility(false);
        } else {
            cursor_backend_set_image(bitmap, width, height, hotspot_x, hotspot_y);
            cursor_backend_set_position(screen_x, screen_y);
            cursor_backend_set_visibility(true);
        }
    }
    
    uint64_t end_tsc = step14_rdtsc();
    cursor_diag_log_render((uint32_t)step14_cycles_to_us(end_tsc - start_tsc), false);
}

void BSPE_CursorPresenter_OnCompositorRedraw(const BVFramebuffer* ram_fb, const BVFramebuffer* hw_fb) {
    (void)hw_fb;
    if (cursor_backend_is_hardware()) return;
    if (!s_state.visible || !ram_fb || !ram_fb->buffer || ram_fb->width == 0 || ram_fb->height == 0) return;
    
    /* V3 Architecture: Single Authoritative Cursor Overlay Pass.
     * Use live PointerState hardware coordinates to guarantee 100% smooth real-time tracking.
     */
    extern const PointerState* pointer_state_get(void);
    const PointerState *ps = pointer_state_get();
    int32_t draw_x = (ps) ? ps->current_x : s_state.current_x;
    int32_t draw_y = (ps) ? ps->current_y : s_state.current_y;

    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(draw_x, draw_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb->width, ram_fb->height, &new_box);
    
    if (!new_box.is_valid) {
        s_prev_box.is_valid = false;
        return;
    }
    
    cp_draw_box(&new_box, ram_fb, s_bitmap, s_state.width, s_state.height, s_state.scale_percent);
    s_prev_box = new_box;
    s_last_union = new_box;
}

void BSPE_CursorPresenter_RestoreBackground(const BVFramebuffer* target_fb) {
    (void)target_fb;
    /* Obsolete in V3 Single-Writer Architecture: Compositor repaints damaged regions natively. */
}

void BSPE_CursorPresenter_BeginComposition(void) {
    if (!g_bspe_cursor_fast_path_enabled || cursor_backend_is_hardware()) return;
    
    extern void* BOVISUAL_Graphics_GetBuffer(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    extern uint32_t BOVISUAL_Graphics_GetPitch(void);
    
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();
    
    /* Erase cursor from backbuffer so compositor doesn't pick it up */
    cp_restore_shadow(&ram_fb);
    
    /* Take ownership lock to block fast path */
    s_compositor_owns_buffer = true;
}

void BSPE_CursorPresenter_EndComposition(void) {
    if (!g_bspe_cursor_fast_path_enabled || cursor_backend_is_hardware()) return;
    
    extern void* BOVISUAL_Graphics_GetBuffer(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    extern uint32_t BOVISUAL_Graphics_GetPitch(void);
    
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();
    
    /* Re-capture clean background and draw cursor overlay */
    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(s_requested_x, s_requested_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb.width, ram_fb.height, &new_box);
    
    if (new_box.is_valid && s_state.visible) {
        cp_capture_shadow(&new_box, &ram_fb);
        cp_draw_box(&new_box, &ram_fb, s_bitmap, s_state.width, s_state.height, s_state.scale_percent);
        s_prev_box = new_box;
    }
    
    /* Release ownership lock */
    s_compositor_owns_buffer = false;
    
    /* Do NOT clear s_cursor_pending here, as this is just the RAM compositor drawing! */
}

extern BSPE_Error BSPE_VRAM_CopyEffectiveDamage(const BOGE_StagingFrame* frame, const BOGE_Rect* effective_rects, uint32_t effective_count);
extern void BWE_AddCompositorDirtyRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

void BSPE_CursorPresenter_PumpFastPath(void) {
    if (!g_bspe_cursor_fast_path_enabled || cursor_backend_is_hardware()) return;
    
    g_cursor_pump_calls++;

    if (s_compositor_owns_buffer) {
        if (s_cursor_pending) {
            extern volatile uint64_t g_cursor_blocked_by_compositor;
            g_cursor_blocked_by_compositor++;
        }
        return;
    }
    
    if (!s_cursor_pending) {
        g_cursor_pump_no_pending++;
        return;
    }
    
    g_cursor_pump_pending_consumed++;
    uint64_t start_tsc = step14_rdtsc();
    uint64_t pending_age_us = step14_cycles_to_us(start_tsc - s_pending_start_tsc);
    
    if (pending_age_us > g_cursor_pending_age_max_us) g_cursor_pending_age_max_us = pending_age_us;
    if (pending_age_us > 50000) g_cursor_pending_over_50ms++;
    else if (pending_age_us > 16000) g_cursor_pending_over_16ms++;
    else if (pending_age_us > 5000) g_cursor_pending_over_5ms++;
    else if (pending_age_us > 2000) g_cursor_pending_over_2ms++;
    
    extern void* BOVISUAL_Graphics_GetBuffer(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    extern uint32_t BOVISUAL_Graphics_GetPitch(void);
    
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();
    if (!ram_fb.buffer) return;
    
    /* Calculate new requested box */
    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(s_requested_x, s_requested_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb.width, ram_fb.height, &new_box);
    
    if (!new_box.is_valid) {
        s_cursor_pending = false;
        return;
    }
    
    BOGE_Rect rects_to_update[2];
    uint32_t update_count = 0;
    
    /* 1. Restore OLD (if valid) */
    if (s_shadow_valid && s_shadow_box.is_valid) {
        cp_restore_shadow(&ram_fb);
        rects_to_update[update_count].x = s_shadow_box.draw_x;
        rects_to_update[update_count].y = s_shadow_box.draw_y;
        rects_to_update[update_count].width = s_shadow_box.draw_w;
        rects_to_update[update_count].height = s_shadow_box.draw_h;
        update_count++;
    }
    
    /* 2. Capture NEW clean background */
    if (s_state.visible) {
        cp_capture_shadow(&new_box, &ram_fb);
        /* 3. Draw NEW cursor overlay */
        cp_draw_box(&new_box, &ram_fb, s_bitmap, s_state.width, s_state.height, s_state.scale_percent);
        
        rects_to_update[update_count].x = new_box.draw_x;
        rects_to_update[update_count].y = new_box.draw_y;
        rects_to_update[update_count].width = new_box.draw_w;
        rects_to_update[update_count].height = new_box.draw_h;
        update_count++;
    }
    
    s_prev_box = new_box;
    
    bool present_success = true;

    /* 4. VRAM Partial Present (Direct Copy) */
    if (update_count > 0) {
        extern BVFramebuffer* vbe_get_back_page_ptr(void);
        BVFramebuffer* back_vram_ptr = vbe_get_back_page_ptr();
        
        extern BVFramebuffer* vbe_get_framebuffer(void);
        BVFramebuffer* front_vram_ptr = vbe_get_framebuffer();

        BOGE_StagingFrame frame = {0};
        frame.width = ram_fb.width;
        frame.height = ram_fb.height;
        frame.pitch = ram_fb.pitch;
        
        /* FIRST: Write to active FRONT BUFFER for immediate zero-latency visibility */
        frame.buffer_virtual_address = front_vram_ptr;
        BSPE_Error err1 = BSPE_VRAM_CopyEffectiveDamage(&frame, rects_to_update, update_count);
        
        /* SECOND: Write to hidden BACK BUFFER to maintain sync for the next Compositor/AGDTE page flip */
        frame.buffer_virtual_address = back_vram_ptr;
        BSPE_Error err2 = BSPE_VRAM_CopyEffectiveDamage(&frame, rects_to_update, update_count);
        
        if (err1 != BSPE_OK || err2 != BSPE_OK) {
            present_success = false;
            
            /* Record exact reason */
            BSPE_Error err = (err1 != BSPE_OK) ? err1 : err2;
            if (err == BSPE_ERR_INVALID_STATE) {
                extern volatile uint64_t g_cursor_fallback_invalid_state;
                g_cursor_fallback_invalid_state++;
            } else {
                extern volatile uint64_t g_cursor_fallback_vram_fail;
                g_cursor_fallback_vram_fail++;
            }

            /* HEALTH CONTRACT: Disable fast path and forcefully recover */
            g_bspe_cursor_fast_path_enabled = false;
            cp_restore_shadow(&ram_fb); /* Erase broken fast-path cursor from RAM */
            s_shadow_valid = false;
            s_prev_box.is_valid = false;

            /* Force legacy compositor to fully redraw everything */
            BWE_AddCompositorDirtyRect(0, 0, ram_fb.width, ram_fb.height);
        }
    }
    
    uint64_t end_tsc = step14_rdtsc();
    uint64_t elapsed_us = step14_cycles_to_us(end_tsc - start_tsc);
    
    if (present_success) {
        extern volatile uint64_t g_cursor_fast_presents;
        extern volatile uint64_t g_cursor_fast_path_total_us;
        extern volatile uint64_t g_cursor_fast_path_max_us;

        g_cursor_fast_presents++;
        g_cursor_fast_path_total_us += elapsed_us;
        if (elapsed_us > g_cursor_fast_path_max_us) {
            g_cursor_fast_path_max_us = elapsed_us;
        }
        
        /* INVARIANT: ONLY clear pending if presentation succeeded */
        s_cursor_pending = false;
        s_state.current_x = s_requested_x;
        s_state.current_y = s_requested_y;
    }
}

void BSPE_CursorPresenter_GetState(BSPE_CursorPresenterState* out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
