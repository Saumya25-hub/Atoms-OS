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

extern void* BOVISUAL_Graphics_GetBuffer(void);
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern BVFramebuffer* vbe_get_framebuffer(void);
extern BVFramebuffer vbe_get_back_page(void);
extern void BOVISUAL_Graphics_AddDamage(int32_t x, int32_t y, int32_t width, int32_t height);

static BSPE_CursorPresenterState s_state;
static uint32_t s_bitmap[64 * 64];
static uint32_t s_shadow[64 * 64];
static CursorBoundingBox s_prev_box;
static CursorBoundingBox s_last_union;

static void cp_restore_box(const CursorBoundingBox* box, const BVFramebuffer* fb) {
    if (!box || !box->is_valid || !fb || !fb->buffer || fb->width == 0 || fb->height == 0) return;
    uint32_t pitch_pixels = fb->pitch / 4;
    if (pitch_pixels == 0) pitch_pixels = fb->width;

    for (uint32_t y = 0; y < box->draw_h; y++) {
        for (uint32_t x = 0; x < box->draw_w; x++) {
            uint32_t screen_idx = (box->draw_y + y) * pitch_pixels + (box->draw_x + x);
            uint32_t shadow_idx = y * box->draw_w + x;
            if (shadow_idx < 64 * 64 && (box->draw_y + y) < fb->height && (box->draw_x + x) < fb->width) {
                fb->buffer[screen_idx] = s_shadow[shadow_idx];
            }
        }
    }
}

static void cp_capture_box(const CursorBoundingBox* box, const BVFramebuffer* fb) {
    if (!box || !box->is_valid || !fb || !fb->buffer || fb->width == 0 || fb->height == 0) return;
    uint32_t pitch_pixels = fb->pitch / 4;
    if (pitch_pixels == 0) pitch_pixels = fb->width;

    for (uint32_t y = 0; y < box->draw_h; y++) {
        for (uint32_t x = 0; x < box->draw_w; x++) {
            uint32_t screen_idx = (box->draw_y + y) * pitch_pixels + (box->draw_x + x);
            uint32_t shadow_idx = y * box->draw_w + x;
            if (shadow_idx < 64 * 64 && (box->draw_y + y) < fb->height && (box->draw_x + x) < fb->width) {
                s_shadow[shadow_idx] = fb->buffer[screen_idx];
            }
        }
    }
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
    s_state.visible = false;
    s_state.scale_percent = 100;
    s_prev_box.is_valid = false;
    s_last_union.is_valid = false;
}

void BSPE_CursorPresenter_UpdatePosition(int32_t screen_x, int32_t screen_y, const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y, bool visible, uint32_t scale_percent) {
    uint64_t start_tsc = step14_rdtsc();
    
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
        uint64_t end_tsc = step14_rdtsc();
        cursor_diag_log_render((uint32_t)step14_cycles_to_us(end_tsc - start_tsc), false);
        return;
    }
    
    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = g_kernel_screen_width;
    ram_fb.height = g_kernel_screen_height;
    ram_fb.pitch = g_kernel_screen_width * 4;
    
    BVFramebuffer* front_vram = vbe_get_framebuffer();
    extern BVFramebuffer* vbe_get_back_page_ptr(void);
    BVFramebuffer* back_vram_ptr = vbe_get_back_page_ptr();
    
    /* 1. Restore previous background on RAM and both VRAM pages */
    if (s_prev_box.is_valid) {
        cp_restore_box(&s_prev_box, &ram_fb);
        if (front_vram) cp_restore_box(&s_prev_box, front_vram);
        cp_restore_box(&s_prev_box, back_vram_ptr);
    }
    
    if (!visible || !bitmap || width == 0 || height == 0) {
        if (s_prev_box.is_valid) {
            BOVISUAL_Graphics_AddDamage(s_prev_box.draw_x, s_prev_box.draw_y, (int32_t)s_prev_box.draw_w, (int32_t)s_prev_box.draw_h);
            s_last_union = s_prev_box;
            s_prev_box.is_valid = false;
        }
        uint64_t end_tsc = step14_rdtsc();
        cursor_diag_log_render((uint32_t)step14_cycles_to_us(end_tsc - start_tsc), true);
        return;
    }
    
    /* 2. Calculate new clamped bounding box */
    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(screen_x, screen_y, width, height, hotspot_x, hotspot_y, s_state.scale_percent, ram_fb.width, ram_fb.height, &new_box);
    
    if (!new_box.is_valid) {
        s_prev_box.is_valid = false;
        uint64_t end_tsc = step14_rdtsc();
        cursor_diag_log_render((uint32_t)step14_cycles_to_us(end_tsc - start_tsc), true);
        return;
    }
    
    /* 3. Capture clean background under new_box from RAM buffer into shadow */
    cp_capture_box(&new_box, &ram_fb);
    
    /* 4. Blit cursor sprite onto RAM and both VRAM pages (< 1 microsecond blit) */
    cp_draw_box(&new_box, &ram_fb, bitmap, width, height, s_state.scale_percent);
    if (front_vram) cp_draw_box(&new_box, front_vram, bitmap, width, height, s_state.scale_percent);
    cp_draw_box(&new_box, back_vram_ptr, bitmap, width, height, s_state.scale_percent);
    
    /* 5. Calculate damage union */
    CursorBoundingBox union_box;
    cursor_hotspot_union_box(&s_prev_box, &new_box, &union_box);
    if (union_box.is_valid) {
        BOVISUAL_Graphics_AddDamage(union_box.draw_x, union_box.draw_y, (int32_t)union_box.draw_w, (int32_t)union_box.draw_h);
        s_last_union = union_box;
    }
    
    s_prev_box = new_box;
    
    uint64_t end_tsc = step14_rdtsc();
    cursor_diag_log_render((uint32_t)step14_cycles_to_us(end_tsc - start_tsc), true);
}

void BSPE_CursorPresenter_OnCompositorRedraw(const BVFramebuffer* ram_fb, const BVFramebuffer* hw_fb) {
    (void)hw_fb;
    if (cursor_backend_is_hardware()) return;
    if (!s_state.visible || !s_prev_box.is_valid || !ram_fb) return;
    
    /* Re-capture shadow background from newly composited windows and apply sprite */
    cp_capture_box(&s_prev_box, ram_fb);
    cp_draw_box(&s_prev_box, ram_fb, s_bitmap, s_state.width, s_state.height, s_state.scale_percent);
}

void BSPE_CursorPresenter_RestoreBackground(const BVFramebuffer* target_fb) {
    if (cursor_backend_is_hardware()) return;
    if (s_prev_box.is_valid && target_fb) {
        cp_restore_box(&s_prev_box, target_fb);
    }
}

void BSPE_CursorPresenter_GetState(BSPE_CursorPresenterState* out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
