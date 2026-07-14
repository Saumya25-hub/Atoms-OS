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
#include "kernel/drivers/input/cursor/cursor_backend.h"
#include "kernel/drivers/input/cursor/cursor_diag.h"
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

void BSPE_CursorPresenter_UpdatePosition(int32_t screen_x, int32_t screen_y, const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y, bool visible, uint32_t scale_percent) {
    uint64_t start_tsc = step14_rdtsc();
    
    /* V3 Architecture: STATE ONLY. Zero framebuffer drawing or shadow manipulation. */
    s_state.current_x = screen_x;
    s_state.current_y = screen_y;
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    // display_print("(7) BSPE_CursorPresenter_UpdatePosition: X="); display_print_dec((uint64_t)screen_x);
    // display_print(" Y="); display_print_dec((uint64_t)screen_y); display_print("\n");
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
     * Compositor window rendering has already repainted the dirty background under the cursor in ram_fb.
     * We calculate the bounding box from current state and overlay the sprite into ram_fb exactly once.
     */
    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(s_state.current_x, s_state.current_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb->width, ram_fb->height, &new_box);
    
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

void BSPE_CursorPresenter_GetState(BSPE_CursorPresenterState* out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
