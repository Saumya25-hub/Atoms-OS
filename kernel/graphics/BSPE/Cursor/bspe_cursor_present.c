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
    BSPE_CursorPresenter_FastTileUpdate();
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

volatile bool g_bcm_compositor_presenting = false;

void BSPE_CursorPresenter_FastTileUpdate(void) {
    if (cursor_backend_is_hardware()) return;
    if (!s_state.visible) return;

    /* 1. Concurrency Check: Yield if Compositor is currently swapping/flipping VRAM */
    if (g_bcm_compositor_presenting) {
        s_cursor_pending = true;
        return;
    }

    extern const PointerState* pointer_state_get(void);
    const PointerState *ps = pointer_state_get();
    int32_t new_x = (ps) ? ps->current_x : s_state.current_x;
    int32_t new_y = (ps) ? ps->current_y : s_state.current_y;

    extern BVFramebuffer* vbe_get_framebuffer(void);
    BVFramebuffer* vram_fb = vbe_get_framebuffer();
    if (!vram_fb || !vram_fb->buffer) return;

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

    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(new_x, new_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb.width, ram_fb.height, &new_box);
    if (!new_box.is_valid) return;

    /* Check if cursor position actually moved */
    if (s_prev_box.is_valid && s_prev_box.draw_x == new_box.draw_x && s_prev_box.draw_y == new_box.draw_y && !s_cursor_pending) {
        return;
    }

    uint32_t screen_w = ram_fb.width;
    uint32_t screen_h = ram_fb.height;
    uint32_t vram_pitch_pixels = vram_fb->pitch / 4;
    if (vram_pitch_pixels == 0) vram_pitch_pixels = screen_w;
    uint32_t ram_pitch_pixels = ram_fb.pitch / 4;
    if (ram_pitch_pixels == 0) ram_pitch_pixels = screen_w;

    /* Step 1: Restore pristine background from RAM to VRAM at old cursor box */
    if (s_prev_box.is_valid) {
        for (uint32_t y = 0; y < s_prev_box.draw_h; y++) {
            uint32_t py = s_prev_box.draw_y + y;
            if (py >= screen_h) break;
            uint32_t ram_row = py * ram_pitch_pixels + s_prev_box.draw_x;
            uint32_t vram_row = py * vram_pitch_pixels + s_prev_box.draw_x;

            for (uint32_t x = 0; x < s_prev_box.draw_w; x++) {
                uint32_t px = s_prev_box.draw_x + x;
                if (px >= screen_w) break;
                vram_fb->buffer[vram_row + x] = ram_fb.buffer[ram_row + x];
            }
        }
    }

    /* Step 2: Draw cursor sprite over pristine RAM background directly onto physical VRAM */
    for (uint32_t y = 0; y < new_box.draw_h; y++) {
        uint32_t py = new_box.draw_y + y;
        if (py >= screen_h) break;
        uint32_t sprite_y = new_box.sprite_offset_y + (y * 100) / s_state.scale_percent;
        if (sprite_y >= s_state.height) sprite_y = s_state.height - 1;

        uint32_t ram_row = py * ram_pitch_pixels + new_box.draw_x;
        uint32_t vram_row = py * vram_pitch_pixels + new_box.draw_x;

        for (uint32_t x = 0; x < new_box.draw_w; x++) {
            uint32_t px = new_box.draw_x + x;
            if (px >= screen_w) break;
            uint32_t sprite_x = new_box.sprite_offset_x + (x * 100) / s_state.scale_percent;
            if (sprite_x >= s_state.width) sprite_x = s_state.width - 1;

            uint32_t argb = s_bitmap[sprite_y * s_state.width + sprite_x];
            uint32_t alpha = (argb >> 24) & 0xFF;
            if (alpha == 0) {
                /* Transparent: restore background from pristine RAM */
                vram_fb->buffer[vram_row + x] = ram_fb.buffer[ram_row + x];
            } else if (alpha == 255) {
                /* Opaque: direct copy */
                vram_fb->buffer[vram_row + x] = argb;
            } else {
                /* Alpha blend with pristine RAM background */
                uint32_t bg = ram_fb.buffer[ram_row + x];
                uint32_t inv_alpha = 255 - alpha;
                uint32_t r = (((argb >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha) / 255;
                uint32_t g = (((argb >> 8)  & 0xFF) * alpha + ((bg >> 8)  & 0xFF) * inv_alpha) / 255;
                uint32_t b = ((argb         & 0xFF) * alpha + (bg         & 0xFF) * inv_alpha) / 255;
                vram_fb->buffer[vram_row + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }

    s_prev_box = new_box;
    s_cursor_pending = false;
    s_state.current_x = new_x;
    s_state.current_y = new_y;
}

void BSPE_CursorPresenter_OnCompositorRedraw(const BVFramebuffer* ram_fb, const BVFramebuffer* hw_fb) {
    if (cursor_backend_is_hardware()) return;
    if (!s_state.visible || !ram_fb || !ram_fb->buffer) return;

    extern BVFramebuffer* vbe_get_framebuffer(void);
    BVFramebuffer* vram_fb = hw_fb ? (BVFramebuffer*)hw_fb : vbe_get_framebuffer();
    if (!vram_fb || !vram_fb->buffer) return;

    extern const PointerState* pointer_state_get(void);
    const PointerState *ps = pointer_state_get();
    int32_t draw_x = (ps) ? ps->current_x : s_state.current_x;
    int32_t draw_y = (ps) ? ps->current_y : s_state.current_y;

    CursorBoundingBox new_box;
    cursor_hotspot_calculate_box(draw_x, draw_y, s_state.width, s_state.height, s_state.hotspot_x, s_state.hotspot_y, s_state.scale_percent, ram_fb->width, ram_fb->height, &new_box);
    if (!new_box.is_valid) return;

    uint32_t screen_w = ram_fb->width;
    uint32_t screen_h = ram_fb->height;
    uint32_t vram_pitch_pixels = vram_fb->pitch / 4;
    if (vram_pitch_pixels == 0) vram_pitch_pixels = screen_w;
    uint32_t ram_pitch_pixels = ram_fb->pitch / 4;
    if (ram_pitch_pixels == 0) ram_pitch_pixels = screen_w;

    /* Overlay cursor onto physical VRAM scanout after window damage pass */
    for (uint32_t y = 0; y < new_box.draw_h; y++) {
        uint32_t py = new_box.draw_y + y;
        if (py >= screen_h) break;
        uint32_t sprite_y = new_box.sprite_offset_y + (y * 100) / s_state.scale_percent;
        if (sprite_y >= s_state.height) sprite_y = s_state.height - 1;

        uint32_t ram_row = py * ram_pitch_pixels + new_box.draw_x;
        uint32_t vram_row = py * vram_pitch_pixels + new_box.draw_x;

        for (uint32_t x = 0; x < new_box.draw_w; x++) {
            uint32_t px = new_box.draw_x + x;
            if (px >= screen_w) break;
            uint32_t sprite_x = new_box.sprite_offset_x + (x * 100) / s_state.scale_percent;
            if (sprite_x >= s_state.width) sprite_x = s_state.width - 1;

            uint32_t argb = s_bitmap[sprite_y * s_state.width + sprite_x];
            uint32_t alpha = (argb >> 24) & 0xFF;
            if (alpha == 255) {
                vram_fb->buffer[vram_row + x] = argb;
            } else if (alpha > 0) {
                uint32_t bg = ram_fb->buffer[ram_row + x];
                uint32_t inv_alpha = 255 - alpha;
                uint32_t r = (((argb >> 16) & 0xFF) * alpha + ((bg >> 16) & 0xFF) * inv_alpha) / 255;
                uint32_t g = (((argb >> 8)  & 0xFF) * alpha + ((bg >> 8)  & 0xFF) * inv_alpha) / 255;
                uint32_t b = ((argb         & 0xFF) * alpha + (bg         & 0xFF) * inv_alpha) / 255;
                vram_fb->buffer[vram_row + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }

    s_prev_box = new_box;
    s_state.current_x = draw_x;
    s_state.current_y = draw_y;
}

void BSPE_CursorPresenter_RestoreBackground(const BVFramebuffer* target_fb) {
    (void)target_fb;
}

void BSPE_CursorPresenter_BeginComposition(void) {
    g_bcm_compositor_presenting = true;
}

void BSPE_CursorPresenter_EndComposition(void) {
    g_bcm_compositor_presenting = false;

    extern void* BOVISUAL_Graphics_GetBuffer(void);
    extern uint32_t BOVISUAL_Graphics_GetWidth(void);
    extern uint32_t BOVISUAL_Graphics_GetHeight(void);
    extern uint32_t BOVISUAL_Graphics_GetPitch(void);

    BVFramebuffer ram_fb;
    ram_fb.buffer = (BOVISUAL_Color*)BOVISUAL_Graphics_GetBuffer();
    ram_fb.width = BOVISUAL_Graphics_GetWidth();
    ram_fb.height = BOVISUAL_Graphics_GetHeight();
    ram_fb.pitch = BOVISUAL_Graphics_GetPitch();

    extern BVFramebuffer* vbe_get_framebuffer(void);
    BVFramebuffer* vram_fb = vbe_get_framebuffer();

    BSPE_CursorPresenter_OnCompositorRedraw(&ram_fb, vram_fb);
}

void BSPE_CursorPresenter_PumpFastPath(void) {
    BSPE_CursorPresenter_FastTileUpdate();
}

void BSPE_CursorPresenter_GetState(BSPE_CursorPresenterState* out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
