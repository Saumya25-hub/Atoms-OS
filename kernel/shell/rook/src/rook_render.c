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

/* Static Double Buffer (Supports up to 2560x1600 resolution) */
static uint32_t  g_rook_backbuffer[2560 * 1600] __attribute__((aligned(16)));
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

static rook_surface_t g_rook_main_surface;

rook_surface_t* rook_get_surface(void) {
    g_rook_main_surface.pixels = rook_get_backbuffer();
    g_rook_main_surface.width = g_fb_width;
    g_rook_main_surface.height = g_fb_height;
    g_rook_main_surface.stride_pixels = g_fb_width; /* Strict Invariant: Stride is ALWAYS Width in RAM */
    g_rook_main_surface.format = 0x01; /* ARGB8888 */
    g_rook_main_surface.is_locked = false;
    return &g_rook_main_surface;
}

void rook_surface_clear(rook_surface_t* surface, uint32_t color) {
    if (!surface || !surface->pixels) return;
    uint32_t total = surface->width * surface->height;
    uint64_t c64 = ((uint64_t)color << 32) | color;
    uint64_t* p64 = (uint64_t*)surface->pixels;
    uint32_t pairs = total >> 1;
    for (uint32_t i = 0; i < pairs; i++) {
        p64[i] = c64;
    }
    if (total & 1) {
        surface->pixels[total - 1] = color;
    }
}

#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/shell/rook/pages/clock_atlas.h"

/*
 * 👑 ARYA COMPOSITOR POINTER HOOK V1.0
 * Real-Time Sub-Pixel Mouse Cursor Blit Layer (Windows DWM / Linux DRM Standard)
 */
static void arya_compositor_draw_cursor(uint32_t* fb, uint32_t width, uint32_t height, uint32_t stride_pixels) {
    if (!fb || width == 0 || height == 0 || stride_pixels == 0) return;

    const PointerState *ps = pointer_state_get();
    if (!ps) return;

    int cur_x = ps->current_x - 2; /* Hotspot X = 2 */
    int cur_y = ps->current_y - 2; /* Hotspot Y = 2 */

    const int cur_w = ARYA_CURSOR_SIZE;
    const int cur_h = ARYA_CURSOR_SIZE;

    for (int y = 0; y < cur_h; y++) {
        int py = cur_y + y;
        if (py < 0 || py >= (int)height) continue;
        uint32_t dst_row = (uint32_t)py * stride_pixels;
        uint32_t src_row = (uint32_t)y * cur_w;

        for (int x = 0; x < cur_w; x++) {
            int px = cur_x + x;
            if (px < 0 || px >= (int)width) continue;

            uint32_t src_pixel = g_arya_cursor_arrow[src_row + x];
            uint8_t a = (uint8_t)(src_pixel >> 24);
            if (a == 0) continue;

            if (a == 255) {
                fb[dst_row + px] = src_pixel & 0x00FFFFFF;
            } else {
                uint32_t dst_pixel = fb[dst_row + px];
                uint32_t inv_a = 255u - a;
                uint32_t r = ((((dst_pixel >> 16) & 0xFFu) * inv_a) + (((src_pixel >> 16) & 0xFFu) * a)) / 255u;
                uint32_t g = ((((dst_pixel >> 8) & 0xFFu) * inv_a) + (((src_pixel >> 8) & 0xFFu) * a)) / 255u;
                uint32_t b = (((dst_pixel & 0xFFu) * inv_a) + ((src_pixel & 0xFFu) * a)) / 255u;
                fb[dst_row + px] = (r << 16) | (g << 8) | b;
            }
        }
    }
}

void rook_cursor_micro_blit(void) {
    if (!g_gop_fb || !g_use_backbuffer) return;
    rook_page_t* current = rook_get_current_page();
    if (!current || current->id != ROOK_PAGE_LOGIN) return;

    const PointerState *ps = pointer_state_get();
    if (!ps) return;

    static int32_t s_last_cur_x = -1;
    static int32_t s_last_cur_y = -1;

    if (s_last_cur_x == ps->current_x && s_last_cur_y == ps->current_y) {
        return; // No motion, nothing to blit
    }

    uint32_t pitch_pixels = (g_fb_stride >= (g_fb_width * 4)) ? (g_fb_stride / 4) : g_fb_stride;
    if (pitch_pixels < g_fb_width) pitch_pixels = g_fb_width;

    const int cur_w = ARYA_CURSOR_SIZE;
    const int cur_h = ARYA_CURSOR_SIZE;
    uint32_t* backbuffer = rook_get_backbuffer();

    // 1. Restore Background under OLD cursor location from pristine Backbuffer
    if (s_last_cur_x >= 0 && s_last_cur_y >= 0) {
        int old_x = s_last_cur_x - 2;
        int old_y = s_last_cur_y - 2;

        for (int y = 0; y < cur_h; y++) {
            int py = old_y + y;
            if (py < 0 || py >= (int)g_fb_height) continue;
            uint32_t vram_row = (uint32_t)py * pitch_pixels;
            uint32_t bb_row   = (uint32_t)py * g_fb_width;

            for (int x = 0; x < cur_w; x++) {
                int px = old_x + x;
                if (px < 0 || px >= (int)g_fb_width) continue;
                g_gop_fb[vram_row + px] = backbuffer[bb_row + px];
            }
        }
    }

    // 2. Draw NEW cursor with Alpha Blending directly into VRAM
    int new_x = ps->current_x - 2;
    int new_y = ps->current_y - 2;

    for (int y = 0; y < cur_h; y++) {
        int py = new_y + y;
        if (py < 0 || py >= (int)g_fb_height) continue;
        uint32_t vram_row = (uint32_t)py * pitch_pixels;
        uint32_t bb_row   = (uint32_t)py * g_fb_width;
        uint32_t src_row  = (uint32_t)y * cur_w;

        for (int x = 0; x < cur_w; x++) {
            int px = new_x + x;
            if (px < 0 || px >= (int)g_fb_width) continue;

            uint32_t src_pixel = g_arya_cursor_arrow[src_row + x];
            uint8_t a = (uint8_t)(src_pixel >> 24);
            if (a == 0) continue;

            if (a == 255) {
                g_gop_fb[vram_row + px] = src_pixel & 0x00FFFFFF;
            } else {
                uint32_t bg_pixel = backbuffer[bb_row + px];
                uint32_t inv_a = 255u - a;
                uint32_t r = ((((bg_pixel >> 16) & 0xFFu) * inv_a) + (((src_pixel >> 16) & 0xFFu) * a)) / 255u;
                uint32_t g = ((((bg_pixel >> 8) & 0xFFu) * inv_a) + (((src_pixel >> 8) & 0xFFu) * a)) / 255u;
                uint32_t b = (((bg_pixel & 0xFFu) * inv_a) + ((src_pixel & 0xFFu) * a)) / 255u;
                g_gop_fb[vram_row + px] = (r << 16) | (g << 8) | b;
            }
        }
    }

    __asm__ volatile("sfence" ::: "memory");
    s_last_cur_x = ps->current_x;
    s_last_cur_y = ps->current_y;
}

void rook_render_flush(void) {
    rook_page_t* current = rook_get_current_page();

    if (!g_gop_fb || g_dirty_count == 0) return;

    uint32_t* target_buf = rook_get_backbuffer();

    if (current && current->ops.on_render) {
        /* Pass logical canvas width as stride to enforce Dense RAM Surface Contract */
        current->ops.on_render(current, target_buf, g_fb_width);
    }

    if (rook_is_debug_overlay_enabled()) {
        rook_debug_render_overlay(target_buf, g_fb_width, g_fb_height, g_fb_stride);
    }

    /* 64-Bit Dual-Pixel Chunk Transfers (2 Pixels per QWORD CPU Store) */
    if (g_use_backbuffer && target_buf != g_gop_fb) {
        uint32_t pitch_pixels = (g_fb_stride >= (g_fb_width * 4)) ? (g_fb_stride / 4) : g_fb_stride;
        if (pitch_pixels < g_fb_width) pitch_pixels = g_fb_width;

        static bool s_logged_flush_metrics = false;
        if (!s_logged_flush_metrics) {
            extern void com1_puts(const char* s);
            com1_puts("[PROBE 6 rook_render_flush] g_fb_width=");
            char num[16]; int pos = 0; uint32_t temp = g_fb_width;
            if (temp == 0) { com1_puts("0"); }
            else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

            com1_puts(" g_fb_height=");
            pos = 0; temp = g_fb_height;
            if (temp == 0) { com1_puts("0"); }
            else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

            com1_puts(" g_fb_stride=");
            pos = 0; temp = g_fb_stride;
            if (temp == 0) { com1_puts("0"); }
            else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

            com1_puts(" pitch_pixels=");
            pos = 0; temp = pitch_pixels;
            if (temp == 0) { com1_puts("0"); }
            else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }
            com1_puts("\r\n");

            s_logged_flush_metrics = true;
        }

        for (uint32_t i = 0; i < g_dirty_count; i++) {
            rook_dirty_rect_t* r = &g_dirty_rects[i];
            uint32_t rect_x = r->x;
            uint32_t rect_w = r->width;
            if (rect_x >= g_fb_width) continue;
            if (rect_x + rect_w > g_fb_width) rect_w = g_fb_width - rect_x;

            for (uint32_t row = 0; row < r->height; row++) {
                uint32_t py = r->y + row;
                if (py >= g_fb_height) break;

                uint32_t src_off = py * g_fb_width + rect_x;
                uint32_t dst_off = py * pitch_pixels + rect_x;

                /* 64-bit uint64_t dual-pixel pair transfers */
                if (((src_off | dst_off) & 1) == 0) {
                    const uint64_t* src64 = (const uint64_t*)&target_buf[src_off];
                    uint64_t* dst64 = (uint64_t*)&g_gop_fb[dst_off];
                    uint32_t pairs = rect_w >> 1;
                    for (uint32_t p = 0; p < pairs; p++) {
                        dst64[p] = src64[p];
                    }
                    if (rect_w & 1) {
                        g_gop_fb[dst_off + rect_w - 1] = target_buf[src_off + rect_w - 1];
                    }
                } else {
                    for (uint32_t c = 0; c < rect_w; c++) {
                        g_gop_fb[dst_off + c] = target_buf[src_off + c];
                    }
                }
            }
        }
    }

    if (current && current->id == ROOK_PAGE_LOGIN) {
        rook_cursor_micro_blit();
    }

    __asm__ volatile("sfence" ::: "memory");
    g_dirty_count = 0;
}

void rook_init_renderer(uint32_t* gop_fb, uint32_t width, uint32_t height, uint32_t stride) {
    g_gop_fb = gop_fb;
    g_fb_width = width;
    g_fb_height = height;
    g_fb_stride = stride;
    
    uint32_t pitch_pixels = (stride >= (width * 4)) ? (stride / 4) : stride;
    if (pitch_pixels < width) pitch_pixels = width;

    /* Instantly wipe 100% of physical VRAM to pure black #000000 (Zero UEFI BIOS leftovers) */
    if (g_gop_fb) {
        uint32_t total_words = pitch_pixels * height;
        uint64_t *vram64 = (uint64_t *)g_gop_fb;
        uint32_t qwords = total_words >> 1;
        for (uint32_t i = 0; i < qwords; i++) {
            vram64[i] = 0x0000000000000000ULL;
        }
        if (total_words & 1) {
            g_gop_fb[total_words - 1] = 0x00000000;
        }
        __asm__ volatile("sfence" ::: "memory");
    }

    if (width * height <= (2560 * 1600)) {
        g_use_backbuffer = true;
        uint64_t *bb64 = (uint64_t *)g_rook_backbuffer;
        uint32_t bb_qwords = (width * height) >> 1;
        for (uint32_t i = 0; i < bb_qwords; i++) {
            bb64[i] = 0x0000000000000000ULL;
        }
    } else {
        g_use_backbuffer = false;
    }
    rook_invalidate_full();
}

void rook_blackout_screen(void) {
    if (g_gop_fb) {
        uint32_t pitch_pixels = (g_fb_stride >= (g_fb_width * 4)) ? (g_fb_stride / 4) : g_fb_stride;
        if (pitch_pixels < g_fb_width) pitch_pixels = g_fb_width;
        uint32_t total = pitch_pixels * g_fb_height;
        for (uint32_t i = 0; i < total; i++) {
            g_gop_fb[i] = 0x00000000;
        }
        __asm__ volatile("sfence" ::: "memory");
    }
}
