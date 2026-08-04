#include "toolbar_view.h"

extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

void toolbar_view_render(const BVFramebuffer* fb, const BOS_Rect* bounds, const char* title) {
    (void)title;
    if (!fb || !bounds) return;

    // Sleek title bar header (#141923)
    BWE_FillRect(fb, bounds->x, bounds->y, bounds->w, bounds->h, 0xFF141923);
}
