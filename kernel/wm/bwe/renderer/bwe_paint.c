#include "../include/bwe.h"
#include "kernel/ui/bofont/bofont.h"

// Expose clip stack status from bwe_compositor.c
extern bool BWE_GetClip(BWE_Rect* out_rect);

// ============================================================
// Clipped Pixel Plotting Core Helper
// ============================================================

static inline void plot_pixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color, const BWE_Rect* clip) {
    if (x >= clip->x && x < clip->x + clip->width && y >= clip->y && y < clip->y + clip->height) {
        if (x >= 0 && x < (int32_t)fb->width && y >= 0 && y < (int32_t)fb->height) {
            fb->buffer[y * (fb->pitch / 4) + x] = color;
        }
    }
}

// ============================================================
// Paint Engine Drawing Primitives
// ============================================================

void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (!fb || w <= 0 || h <= 0) return;

    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    int32_t x1 = x;
    int32_t y1 = y;
    int32_t x2 = x + w;
    int32_t y2 = y + h;

    // Bounds clipping intersection check
    if (x1 < clip.x) x1 = clip.x;
    if (y1 < clip.y) y1 = clip.y;
    if (x2 > clip.x + clip.width) x2 = clip.x + clip.width;
    if (y2 > clip.y + clip.height) y2 = clip.y + clip.height;

    if (x1 >= x2 || y1 >= y2) return;

    uint32_t pitch_w = fb->pitch / 4;
    extern void heap_check_external_write(uint64_t dst_addr, size_t len, const char* caller, uint64_t rip);
    if (fb->buffer) heap_check_external_write((uint64_t)(&fb->buffer[y1 * pitch_w + x1]), (y2 - y1) * pitch_w * 4, "BWE_FillRect", (uint64_t)__builtin_return_address(0));
    for (int32_t cy = y1; cy < y2; cy++) {
        uint32_t offset = cy * pitch_w;
        for (int32_t cx = x1; cx < x2; cx++) {
            fb->buffer[offset + cx] = color;
        }
    }
}

void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness) {
    if (!fb || w <= 0 || h <= 0 || thickness == 0) return;

    // Top edge
    BWE_FillRect(fb, x, y, w, (int32_t)thickness, color);
    // Bottom edge
    BWE_FillRect(fb, x, y + h - (int32_t)thickness, w, (int32_t)thickness, color);
    // Left edge
    BWE_FillRect(fb, x, y, (int32_t)thickness, h, color);
    // Right edge
    BWE_FillRect(fb, x + w - (int32_t)thickness, y, (int32_t)thickness, h, color);
}

void BWE_DrawLine(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    if (!fb) return;

    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    int32_t dx = (x2 - x1 >= 0) ? (x2 - x1) : (x1 - x2);
    int32_t dy = (y2 - y1 >= 0) ? (y2 - y1) : (y1 - y2);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    while (1) {
        plot_pixel(fb, x1, y1, color, &clip);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font) {
    (void)fb;
    (void)font;

    // Use default system typography engine (BOFONT)
    BOFont* sys_font = BOFont_GetDefault();
    if (sys_font) {
        BOFont_DrawText(sys_font, text, x, y, color);
    }
}

void BWE_DrawBitmap(const BVFramebuffer* fb, const uint32_t* pixels, int32_t dest_x, int32_t dest_y, int32_t dest_w, int32_t dest_h, int32_t src_x, int32_t src_y, int32_t src_w, int32_t src_h, int32_t bmp_pitch) {
    if (!fb || !pixels || dest_w <= 0 || dest_h <= 0 || src_w <= 0 || src_h <= 0) return;

    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    uint32_t pitch_words = (uint32_t)bmp_pitch / 4;

    for (int32_t dy = 0; dy < dest_h; dy++) {
        int32_t sy = src_y + (dy * src_h) / dest_h;
        int32_t py = dest_y + dy;
        for (int32_t dx = 0; dx < dest_w; dx++) {
            int32_t sx = src_x + (dx * src_w) / dest_w;
            int32_t px = dest_x + dx;

            uint32_t color = pixels[sy * pitch_words + sx];
            // Skip transparent pixels
            if ((color >> 24) != 0) {
                plot_pixel(fb, px, py, color, &clip);
            }
        }
    }
}

void BWE_DrawBorder(const BVFramebuffer* fb, const BWE_Rect* bounds, uint32_t color, bool active) {
    // 5px thick border around bounds
    BWE_DrawRect(fb, bounds->x, bounds->y, bounds->width, bounds->height, color, 5);

    // Draw inner metallic trim border (classic silver styling)
    uint32_t trim_color = active ? 0xFF0038A8 : 0xFF94A3B8;
    BWE_DrawRect(fb, bounds->x + 4, bounds->y + 4, bounds->width - 8, bounds->height - 8, trim_color, 1);
}

void BWE_DrawShadow(const BVFramebuffer* fb, const BWE_Rect* bounds) {
    // Renders soft drop shadow on bottom (8px) and right (8px) of window bounds.
    BWE_Rect clip;
    if (!BWE_GetClip(&clip)) {
        clip.x = 0;
        clip.y = 0;
        clip.width = (int32_t)fb->width;
        clip.height = (int32_t)fb->height;
    }

    // Right shadow
    int32_t rx = bounds->x + bounds->width;
    int32_t ry = bounds->y + 8;
    int32_t rw = 8;
    int32_t rh = bounds->height;
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            plot_pixel(fb, x, y, 0x3F000000, &clip); // Alpha black blend
        }
    }

    // Bottom shadow
    int32_t bx = bounds->x + 8;
    int32_t by = bounds->y + bounds->height;
    int32_t bw = bounds->width;
    int32_t bh = 8;
    for (int32_t y = by; y < by + bh; y++) {
        for (int32_t x = bx; x < bx + bw; x++) {
            plot_pixel(fb, x, y, 0x3F000000, &clip);
        }
    }
}

void BWE_DrawTitleBar(const BVFramebuffer* fb, const BWE_Rect* bounds, const char* title, bool active, bool resizable) {
    // Title bar is 30px tall, starts after the 5px border.
    int32_t tx = bounds->x + 5;
    int32_t ty = bounds->y + 5;
    int32_t tw = bounds->width - 10;
    int32_t th = 30;

    // Linear blue gradient for active state, linear dark gray gradient for inactive state
    uint32_t color_top = active ? 0xFF0058EE : 0xFF64748B;
    uint32_t color_bottom = active ? 0xFF0038A8 : 0xFF475569;

    for (int32_t y = 0; y < th; y++) {
        uint32_t r1 = (color_top >> 16) & 0xFF;
        uint32_t g1 = (color_top >> 8) & 0xFF;
        uint32_t b1 = color_top & 0xFF;
        
        uint32_t r2 = (color_bottom >> 16) & 0xFF;
        uint32_t g2 = (color_bottom >> 8) & 0xFF;
        uint32_t b2 = color_bottom & 0xFF;
        
        uint32_t r = r1 + ((r2 - r1) * y) / th;
        uint32_t g = g1 + ((g2 - g1) * y) / th;
        uint32_t b = b1 + ((b2 - b1) * y) / th;
        
        uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
        BWE_FillRect(fb, tx, ty + y, tw, 1, color);
    }

    // Render title string
    if (title) {
        BWE_DrawText(fb, title, tx + 10, ty + 7, 0xFFFFFFFF, 0);
    }

    // Render control button placeholders (Close, Maximize, Minimize)
    int32_t btn_size = 20;
    int32_t btn_y = ty + 5;

    // 1. Close Button ('X')
    int32_t close_x = tx + tw - 25;
    BWE_FillRect(fb, close_x, btn_y, btn_size, btn_size, 0xFFEF4444); // Red
    BWE_DrawRect(fb, close_x, btn_y, btn_size, btn_size, 0xFFFFFFFF, 1);
    BWE_DrawText(fb, "X", close_x + 6, btn_y + 3, 0xFFFFFFFF, 0);

    // 2. Maximize Button ('O')
    if (resizable) {
        int32_t max_x = tx + tw - 50;
        BWE_FillRect(fb, max_x, btn_y, btn_size, btn_size, 0xFF3B82F6); // Blue
        BWE_DrawRect(fb, max_x, btn_y, btn_size, btn_size, 0xFFFFFFFF, 1);
        BWE_DrawText(fb, "O", max_x + 5, btn_y + 3, 0xFFFFFFFF, 0);
    }

    // 3. Minimize Button ('_')
    int32_t min_x = tx + tw - (resizable ? 75 : 50);
    BWE_FillRect(fb, min_x, btn_y, btn_size, btn_size, 0xFF10B981); // Green
    BWE_DrawRect(fb, min_x, btn_y, btn_size, btn_size, 0xFFFFFFFF, 1);
    BWE_DrawText(fb, "_", min_x + 6, btn_y + 2, 0xFFFFFFFF, 0);
}
