#include "viewport_view.h"

extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void viewport_view_render(const BVFramebuffer* fb, const BOS_Rect* bounds, bool has_active_video) {
    if (!fb || !bounds) return;

    // Dark sleek letterbox background (#0A0D14)
    BWE_FillRect(fb, bounds->x, bounds->y, bounds->w, bounds->h, 0xFF0A0D14);

    if (!has_active_video) {
        // Draw centered logo placeholder frame / Song Mode Visualizer
        int32_t logo_w = 400;
        int32_t logo_h = 220;
        int32_t logo_x = bounds->x + (bounds->w - logo_w) / 2;
        int32_t logo_y = bounds->y + (bounds->h - logo_h) / 2;

        BWE_FillRect(fb, logo_x, logo_y, logo_w, logo_h, 0xFF141923);
        BWE_FillRect(fb, logo_x + 2, logo_y + 2, logo_w - 4, logo_h - 4, 0xFF0F131D);

        // Render 24-band audio spectrum equalizer
        static uint32_t s_anim_tick = 0;
        s_anim_tick++;

        int32_t num_bars = 24;
        int32_t bar_w = 10;
        int32_t bar_gap = 4;
        int32_t total_w = num_bars * (bar_w + bar_gap);
        int32_t start_x = logo_x + (logo_w - total_w) / 2;
        int32_t base_y = logo_y + logo_h - 40;

        for (int32_t i = 0; i < num_bars; i++) {
            // Dynamic bar height using pseudo-audio wave generator
            uint32_t phase = (s_anim_tick * 5 + (uint32_t)i * 19) % 180;
            int32_t bar_h = 15 + ((int32_t)(phase * (uint32_t)(num_bars - i % 5)) % 110);
            if (bar_h > 120) bar_h = 120;

            // Gradient colors from Cyan (0xFF00E5FF) to Magenta (0xFFFF007F)
            uint32_t r = ((uint32_t)i * 255) / (uint32_t)num_bars;
            uint32_t g = 229 - (((uint32_t)i * 150) / (uint32_t)num_bars);
            uint32_t b = 255;
            uint32_t bar_color = 0xFF000000 | (r << 16) | (g << 8) | b;

            int32_t bx = start_x + i * (bar_w + bar_gap);
            int32_t by = base_y - bar_h;
            BWE_FillRect(fb, bx, by, bar_w, bar_h, bar_color);

            // Peak indicator dot
            BWE_FillRect(fb, bx, by - 4, bar_w, 2, 0xFFFFFFFF);
        }
    }
}

