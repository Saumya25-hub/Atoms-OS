#include "framework/include/bos_ui_rendering.h"

void BOS_DrawBorderRect(BOS_DrawContext* ctx, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t thickness, uint32_t radius) {
    if (!ctx || thickness == 0) return;
    (void)radius;

    /* Top border */
    BOS_DrawFillRect(ctx, x, y, w, thickness, color);
    /* Bottom border */
    BOS_DrawFillRect(ctx, x, y + h - thickness, w, thickness, color);
    /* Left border */
    BOS_DrawFillRect(ctx, x, y, thickness, h, color);
    /* Right border */
    BOS_DrawFillRect(ctx, x + w - thickness, y, thickness, h, color);
}

void BOS_DrawText(BOS_DrawContext* ctx, const char* text, int32_t x, int32_t y, uint32_t color) {
    if (!ctx || !text) return;
    (void)x; (void)y; (void)color;
    /* Typography rasterizer hook */
}
