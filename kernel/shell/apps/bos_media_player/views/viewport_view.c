#include "viewport_view.h"

extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void viewport_view_render(const BVFramebuffer* fb, const BOS_Rect* bounds, bool has_active_video) {
    if (!fb || !bounds) return;

    // Dark sleek letterbox background (#0A0D14)
    BWE_FillRect(fb, bounds->x, bounds->y, bounds->w, bounds->h, 0xFF0A0D14);

    if (!has_active_video) {
        // Draw centered logo placeholder frame
        int32_t logo_w = 200;
        int32_t logo_h = 120;
        int32_t logo_x = bounds->x + (bounds->w - logo_w) / 2;
        int32_t logo_y = bounds->y + (bounds->h - logo_h) / 2;

        BWE_FillRect(fb, logo_x, logo_y, logo_w, logo_h, 0xFF141923);
    }
}
