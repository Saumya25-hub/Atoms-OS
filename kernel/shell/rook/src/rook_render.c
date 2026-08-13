#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_debug.h"
#include "kernel/core/lib/include/string.h"

/*
 * ♜ ROOK ENGINE V1.0 — Render Engine & Dirty Rectangle Management
 */

static uint32_t* g_gop_fb = 0;
static uint32_t  g_fb_width = 0;
static uint32_t  g_fb_height = 0;
static uint32_t  g_fb_stride = 0;

/* Static Double Buffer (Max 1920x1080 resolution) */
static uint32_t  g_rook_backbuffer[1920 * 1080] __attribute__((aligned(16)));
static bool      g_use_backbuffer = false;

static rook_dirty_rect_t g_dirty_rects[ROOK_MAX_DIRTY_RECTS];
static uint32_t          g_dirty_count = 0;

void rook_invalidate_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (g_dirty_count >= ROOK_MAX_DIRTY_RECTS) {
        rook_invalidate_full();
        return;
    }
    if (x >= g_fb_width || y >= g_fb_height) return;
    if (x + w > g_fb_width) w = g_fb_width - x;
    if (y + h > g_fb_height) h = g_fb_height - y;

    g_dirty_rects[g_dirty_count].x = x;
    g_dirty_rects[g_dirty_count].y = y;
    g_dirty_rects[g_dirty_count].width = w;
    g_dirty_rects[g_dirty_count].height = h;
    g_dirty_count++;
}

void rook_invalidate_full(void) {
    g_dirty_count = 1;
    g_dirty_rects[0].x = 0;
    g_dirty_rects[0].y = 0;
    g_dirty_rects[0].width = g_fb_width;
    g_dirty_rects[0].height = g_fb_height;
}

uint32_t* rook_get_backbuffer(void) {
    return g_use_backbuffer ? g_rook_backbuffer : g_gop_fb;
}

uint32_t rook_get_width(void) { return g_fb_width; }
uint32_t rook_get_height(void) { return g_fb_height; }
uint32_t rook_get_stride(void) { return g_fb_stride; }

void rook_render_flush(void) {
    if (!g_gop_fb || g_dirty_count == 0) return;

    rook_page_t* current = rook_get_current_page();
    uint32_t* target_buf = rook_get_backbuffer();

    if (current && current->ops.on_render) {
        current->ops.on_render(current, target_buf, g_fb_stride);
    }

    if (rook_is_debug_overlay_enabled()) {
        rook_debug_render_overlay(target_buf, g_fb_width, g_fb_height, g_fb_stride);
    }

    /* Single Atomic Blit dirty rectangles if using backbuffer */
    if (g_use_backbuffer && target_buf != g_gop_fb) {
        uint32_t pitch_pixels = g_fb_stride / 4;
        if (pitch_pixels == 0) pitch_pixels = g_fb_width;

        for (uint32_t i = 0; i < g_dirty_count; i++) {
            rook_dirty_rect_t* r = &g_dirty_rects[i];
            for (uint32_t row = 0; row < r->height; row++) {
                uint32_t py = r->y + row;
                uint32_t src_offset = py * g_fb_width + r->x;
                uint32_t dst_offset = py * pitch_pixels + r->x;
                for (uint32_t col = 0; col < r->width; col++) {
                    g_gop_fb[dst_offset + col] = target_buf[src_offset + col];
                }
            }
        }
    }

    g_dirty_count = 0;
}

void rook_init_renderer(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride) {
    g_gop_fb = gop_fb;
    g_fb_width = width;
    g_fb_height = height;
    g_fb_stride = stride;
    
    uint32_t pitch_pixels = stride / 4;
    if (pitch_pixels == 0) pitch_pixels = width;

    /* Instantly wipe physical VRAM framebuffer to 100% pure black #000000 */
    if (g_gop_fb) {
        uint32_t total_vram_words = pitch_pixels * height;
        if (total_vram_words > (1920 * 1080)) total_vram_words = 1920 * 1080;
        for (uint32_t i = 0; i < total_vram_words; i++) {
            g_gop_fb[i] = 0x00000000;
        }
    }

    if (width * height <= (1920 * 1080)) {
        g_use_backbuffer = true;
        for (uint32_t i = 0; i < width * height; i++) {
            g_rook_backbuffer[i] = 0x00000000;
        }
    } else {
        g_use_backbuffer = false;
    }
    rook_invalidate_full();
}
