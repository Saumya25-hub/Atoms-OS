#include "../include/bosfsr_graphics.h"

void bosfsr_draw_rounded_rect(const BVFramebuffer* fb, int x, int y, int w, int h, int radius, uint32_t fill_color, uint32_t border_color, int border_w) {
    if (!fb || !fb->buffer || w <= 0 || h <= 0) return;

    // Fill body
    BWE_FillRect(fb, x, y, w, h, fill_color);

    // Draw border if specified
    if (border_w > 0 && border_color != 0x00000000) {
        BWE_DrawRect(fb, x, y, w, h, border_color, border_w);
    }
}

void bosfsr_draw_linear_gradient(const BVFramebuffer* fb, int x, int y, int w, int h, uint32_t start_col, uint32_t end_col, bool vertical) {
    if (!fb || !fb->buffer || w <= 0 || h <= 0) return;

    if (vertical) {
        for (int i = 0; i < h; i++) {
            float t = (float)i / (float)(h > 1 ? h - 1 : 1);
            uint8_t r = (uint8_t)(((start_col >> 16) & 0xFF) * (1.0f - t) + ((end_col >> 16) & 0xFF) * t);
            uint8_t g = (uint8_t)(((start_col >> 8) & 0xFF) * (1.0f - t) + ((end_col >> 8) & 0xFF) * t);
            uint8_t b = (uint8_t)((start_col & 0xFF) * (1.0f - t) + (end_col & 0xFF) * t);
            uint32_t col = 0xFF000000 | (r << 16) | (g << 8) | b;
            BWE_FillRect(fb, x, y + i, w, 1, col);
        }
    } else {
        for (int i = 0; i < w; i++) {
            float t = (float)i / (float)(w > 1 ? w - 1 : 1);
            uint8_t r = (uint8_t)(((start_col >> 16) & 0xFF) * (1.0f - t) + ((end_col >> 16) & 0xFF) * t);
            uint8_t g = (uint8_t)(((start_col >> 8) & 0xFF) * (1.0f - t) + ((end_col >> 8) & 0xFF) * t);
            uint8_t b = (uint8_t)((start_col & 0xFF) * (1.0f - t) + (end_col & 0xFF) * t);
            uint32_t col = 0xFF000000 | (r << 16) | (g << 8) | b;
            BWE_FillRect(fb, x + i, y, 1, h, col);
        }
    }
}

void bosfsr_draw_shadow(const BVFramebuffer* fb, int x, int y, int w, int h, int radius) {
    if (!fb || !fb->buffer) return;
    BWE_FillRect(fb, x + 4, y + 4, w, h, 0x40000000);
}
