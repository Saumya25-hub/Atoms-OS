/**
 * @file cursor_theme.c
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Theme Registry Implementation
 * @section PURPOSE
 * Statically allocates and generates professional OS cursor bitmaps (Arrow, Text Beam,
 * Resize, Busy, Wait, Crosshair, Hand) in kernel BSS memory without heap allocation.
 */

#include "cursor_theme.h"
#include <stddef.h>

/* Statically allocated theme registry in BSS */
static CursorThemeSprite g_theme_sprites[CURSOR_SHAPE_MAX];

/* --- Helper to set a pixel in ARGB --- */
static inline void set_pixel(uint32_t* bmp, uint32_t w, uint32_t x, uint32_t y, uint32_t argb) {
    if (x < w && y < CURSOR_THEME_MAX_DIM) {
        bmp[y * w + x] = argb;
    }
}

/* --- Helper to draw horizontal line --- */
static void draw_hline(uint32_t* bmp, uint32_t w, uint32_t x0, uint32_t x1, uint32_t y, uint32_t argb) {
    for (uint32_t x = x0; x <= x1; x++) {
        set_pixel(bmp, w, x, y, argb);
    }
}

/* --- Helper to draw vertical line --- */
static void draw_vline(uint32_t* bmp, uint32_t w, uint32_t x, uint32_t y0, uint32_t y1, uint32_t argb) {
    for (uint32_t y = y0; y <= y1; y++) {
        set_pixel(bmp, w, x, y, argb);
    }
}

/* --- Shape Generators --- */

/* --- Exact ARGB Pixels Extracted from mouse32/arrow.cur --- */
/* Exact 32x32 ARGB array extracted from D:\\Signatures_OS\\mouse32\\arrow.cur */
static const uint32_t s_arrow_cur_pixels[32 * 32] = {
    0x24E7E7E7, 0xBDF8F8F8, 0xFAFAFAFA, 0xD3F8F8F8, 0x44E0E0E0, 0x0F000000, 0x0A000000, 0x05000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xC0F8F8F8, 0xFFB7B7B7, 0xFF3B3B3B, 0xFF8C8C8C, 0xF2F9F9F9, 0x50DBDBDB, 0x14000000, 0x0C000000, 0x05000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFCFAFAFA, 0xFF373737, 0xFF000000, 0xFF000000, 0xFF7C7C7C, 0xF6F9F9F9, 0x5ADBDBDB, 0x15000000, 0x0C000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF7C7C7C, 0xF6F9F9F9, 0x5DDBDBDB, 0x16000000, 0x0C000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF7C7C7C, 0xF8F9F9F9, 0x60DDDDDD, 0x16000000, 0x0C000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF777777, 0xF8F9F9F9, 0x60DDDDDD, 0x16000000, 0x0C000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF757575, 0xF8F9F9F9, 0x60DDDDDD, 0x16000000, 0x0C000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF686868, 0xF8F8F8F8, 0x62DDDDDD, 0x16000000, 0x0C000000, 0x05000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF666666, 0xFAF8F8F8, 0x59DADADA, 0x14000000, 0x0A000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF8E8E8E, 0xC2F4F4F4, 0x1B000000, 0x0F000000, 0x06000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF7F7F7F, 0xC6F4F4F4, 0x20000000, 0x11000000, 0x07000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF040404, 0xFF3B3B3B, 0xFF747474, 0xFF9E9E9E, 0xFEF4F4F4, 0x70D6D6D6, 0x1F000000, 0x11000000, 0x07000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF090909, 0xFF737373, 0xFFDFDFDF, 0xF8F9F9F9, 0xD0F2F2F2, 0xAFEBEBEB, 0x67CFCFCF, 0x26000000, 0x1A000000, 0x0E000000, 0x06000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFF9F9F9, 0xFF2F2F2F, 0xFF000000, 0xFF000000, 0xFF333333, 0xFFD5D5D5, 0xE5F6F6F6, 0x87D8D8D8, 0x3F757575, 0x30000000, 0x2A000000, 0x23000000, 0x1B000000, 0x11000000, 0x09000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xF0F9F9F9, 0xFF555555, 0xFF000000, 0xFF464646, 0xFFF0F0F0, 0xAEE9E9E9, 0x3D626262, 0x2E000000, 0x26000000, 0x20000000, 0x1A000000, 0x14000000, 0x0F000000, 0x09000000, 0x04000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x85F0F0F0, 0xFFEFEFEF, 0xFFBBBBBB, 0xFFF2F2F2, 0x95E2E2E2, 0x31000000, 0x28000000, 0x1F000000, 0x17000000, 0x11000000, 0x0C000000, 0x09000000, 0x06000000, 0x03000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x10000000, 0x59DEDEDE, 0x8CE9E9E9, 0x5CCDCDCD, 0x2A000000, 0x23000000, 0x1A000000, 0x12000000, 0x0B000000, 0x07000000, 0x04000000, 0x02000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0A000000, 0x12000000, 0x18000000, 0x1B000000, 0x1A000000, 0x15000000, 0x0E000000, 0x08000000, 0x03000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05000000, 0x09000000, 0x0C000000, 0x0E000000, 0x0D000000, 0x09000000, 0x06000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x01000000, 0x03000000, 0x04000000, 0x05000000, 0x04000000, 0x03000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static void init_arrow_sprite(void) {
    CursorThemeSprite* s = &g_theme_sprites[CURSOR_SHAPE_ARROW];
    s->width = 32;
    s->height = 32;
    s->hotspot_x = 2;
    s->hotspot_y = 2;
    s->frame_count = 8;
    s->frame_interval_ms = 70;
    s->is_animated = true;

    uint32_t cyan_dot = 0xFF00E5FF; // Glowing Cyan animated wait dot
    const int dot_centers_x[8] = { 20, 22, 23, 22, 20, 18, 17, 18 };
    const int dot_centers_y[8] = { 10, 12, 14, 16, 17, 16, 14, 12 };

    for (uint32_t f = 0; f < 8; f++) {
        uint32_t* bmp = s->bitmaps[f];

        /* 1. Copy 100% exact ARGB pixels from D:\Signatures_OS\mouse32\arrow.cur */
        for (uint32_t i = 0; i < 32 * 32; i++) {
            bmp[i] = s_arrow_cur_pixels[i];
        }

        /* 2. Overlay animated cyan wait dot */
        int cx = dot_centers_x[f];
        int cy = dot_centers_y[f];
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < 32 && py >= 0 && py < 32) {
                    set_pixel(bmp, 32, px, py, cyan_dot);
                }
            }
        }
    }
}

static void init_text_beam_sprite(void) {
    CursorThemeSprite* s = &g_theme_sprites[CURSOR_SHAPE_TEXT_BEAM];
    s->width = 16;
    s->height = 24;
    s->hotspot_x = 8;
    s->hotspot_y = 12;
    s->frame_count = 1;
    s->is_animated = false;

    uint32_t* bmp = s->bitmaps[0];
    for (uint32_t i = 0; i < s->width * s->height; i++) bmp[i] = 0x00000000;

    uint32_t white = 0xFFFFFFFF;
    uint32_t black = 0xFF000000;

    /* Vertical stem with shadow */
    draw_vline(bmp, s->width, 8, 3, 20, white);
    draw_vline(bmp, s->width, 7, 3, 20, black);
    draw_vline(bmp, s->width, 9, 3, 20, black);

    /* Top and bottom horizontal serif caps */
    draw_hline(bmp, s->width, 5, 11, 3, white);
    draw_hline(bmp, s->width, 5, 11, 2, black);
    draw_hline(bmp, s->width, 5, 11, 4, black);
    draw_hline(bmp, s->width, 5, 11, 20, white);
    draw_hline(bmp, s->width, 5, 11, 19, black);
    draw_hline(bmp, s->width, 5, 11, 21, black);
}

static void init_resize_sprites(void) {
    /* Horizontal Resize (<--->) */
    CursorThemeSprite* sh = &g_theme_sprites[CURSOR_SHAPE_RESIZE_H];
    sh->width = 24;
    sh->height = 16;
    sh->hotspot_x = 12;
    sh->hotspot_y = 8;
    sh->frame_count = 1;
    sh->is_animated = false;

    uint32_t* bmph = sh->bitmaps[0];
    for (uint32_t i = 0; i < sh->width * sh->height; i++) bmph[i] = 0x00000000;
    draw_hline(bmph, sh->width, 4, 19, 8, 0xFFFFFFFF);
    draw_hline(bmph, sh->width, 4, 19, 7, 0xFF000000);
    draw_hline(bmph, sh->width, 4, 19, 9, 0xFF000000);
    /* Left arrow head */
    set_pixel(bmph, sh->width, 3, 7, 0xFFFFFFFF); set_pixel(bmph, sh->width, 3, 9, 0xFFFFFFFF);
    set_pixel(bmph, sh->width, 2, 8, 0xFFFFFFFF);
    /* Right arrow head */
    set_pixel(bmph, sh->width, 20, 7, 0xFFFFFFFF); set_pixel(bmph, sh->width, 20, 9, 0xFFFFFFFF);
    set_pixel(bmph, sh->width, 21, 8, 0xFFFFFFFF);

    /* Vertical Resize */
    CursorThemeSprite* sv = &g_theme_sprites[CURSOR_SHAPE_RESIZE_V];
    sv->width = 16;
    sv->height = 24;
    sv->hotspot_x = 8;
    sv->hotspot_y = 12;
    sv->frame_count = 1;
    sv->is_animated = false;

    uint32_t* bmpv = sv->bitmaps[0];
    for (uint32_t i = 0; i < sv->width * sv->height; i++) bmpv[i] = 0x00000000;
    draw_vline(bmpv, sv->width, 8, 4, 19, 0xFFFFFFFF);
    draw_vline(bmpv, sv->width, 7, 4, 19, 0xFF000000);
    draw_vline(bmpv, sv->width, 9, 4, 19, 0xFF000000);
    /* Top arrow head */
    set_pixel(bmpv, sv->width, 7, 3, 0xFFFFFFFF); set_pixel(bmpv, sv->width, 9, 3, 0xFFFFFFFF);
    set_pixel(bmpv, sv->width, 8, 2, 0xFFFFFFFF);
    /* Bottom arrow head */
    set_pixel(bmpv, sv->width, 7, 20, 0xFFFFFFFF); set_pixel(bmpv, sv->width, 9, 20, 0xFFFFFFFF);
    set_pixel(bmpv, sv->width, 8, 21, 0xFFFFFFFF);
}

static void init_animated_sprites(void) {
    /* Busy Spinner (8 animated frames) */
    CursorThemeSprite* sb = &g_theme_sprites[CURSOR_SHAPE_BUSY];
    sb->width = 24;
    sb->height = 24;
    sb->hotspot_x = 12;
    sb->hotspot_y = 12;
    sb->frame_count = 8;
    sb->frame_interval_ms = 100;
    sb->is_animated = true;

    const char* spinner_frames[8][24] = {
        {
            "           B            ",
            "          BDB           ",
            "         BDDDB          ",
            "    BBB  BDDDB  BBB     ",
            "   BWWBB BDDDB BBWWB    ",
            "   BWWWBBBDDDBBBWWWB    ",
            "   BBWWWBBDDDBBWWWBB    ",
            "    BBWWWBBDBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBWBBWWWBB     ",
            "   BBWWWBBWWWBBWWWBB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BWWBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBDDB    ",
            "   BWWWBBBWWWBBBDDDB    ",
            "   BBWWWBBWWWBBDDDBB    ",
            "    BBWWWBBWBBDDDBB     ",
            "     BBWWB B BDDBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBWBBWWWBB     ",
            "   BBWWWBBWWWBBWWWBB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BWWBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBWWB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BBWWWBBWWWBBWWWBB    ",
            "    BBWWWBBWBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BDDDDDB  ",
            "BWWWWWWWB     BDDDDDDDB ",
            " BWWWWWB       BDDDDDB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBWBBWWWBB     ",
            "   BBWWWBBWWWBBWWWBB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BWWBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBWWB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BBWWWBBWWWBBWWWBB    ",
            "    BBWWWBBWBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BDDBB      ",
            "    BBWWWBBWBBDDDBB     ",
            "   BBWWWBBWWWBBDDDBB    ",
            "   BWWWBBBWWWBBBDDDB    ",
            "   BWWBB BWWWB BBDDB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBWWB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BBWWWBBWWWBBWWWBB    ",
            "    BBWWWBBWBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBDBBWWWBB     ",
            "   BBWWWBBDDDBBWWWBB    ",
            "   BWWWBBBDDDBBBWWWB    ",
            "   BWWBB BDDDB BBWWB    ",
            "    BBB  BDDDB  BBB     ",
            "         BDDDB          ",
            "          BDB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBWWB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BBWWWBBWWWBBWWWBB    ",
            "    BBWWWBBWBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBDDB B BWWBB      ",
            "    BBDDDBBWBBWWWBB     ",
            "   BBDDDBBWWWBBWWWBB    ",
            "   BDDDBBBWWWBBBWWWB    ",
            "   BDDBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BWWBB BWWWB BBWWB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BBWWWBBWWWBBWWWBB    ",
            "    BBWWWBBWBBWWWBB     ",
            "     BBWWB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BDDDDDB       BWWWWWB  ",
            "BDDDDDDDB     BWWWWWWWB ",
            " BDDDDDB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBWBBWWWBB     ",
            "   BBWWWBBWWWBBWWWBB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BWWBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
        {
            "           B            ",
            "          BWB           ",
            "         BWWWB          ",
            "    BBB  BWWWB  BBB     ",
            "   BDDBB BWWWB BBWWB    ",
            "   BDDDBBBWWWBBBWWWB    ",
            "   BBDDDBBWWWBBWWWBB    ",
            "    BBDDDBBWBBWWWBB     ",
            "     BBDDB B BWWBB      ",
            "  BBBBBBB     BBBBBBB   ",
            " BWWWWWB       BWWWWWB  ",
            "BWWWWWWWB     BWWWWWWWB ",
            " BWWWWWB       BWWWWWB  ",
            "  BBBBBBB     BBBBBBB   ",
            "     BBWWB B BWWBB      ",
            "    BBWWWBBWBBWWWBB     ",
            "   BBWWWBBWWWBBWWWBB    ",
            "   BWWWBBBWWWBBBWWWB    ",
            "   BWWBB BWWWB BBWWB    ",
            "    BBB  BWWWB  BBB     ",
            "         BWWWB          ",
            "          BWB           ",
            "           B            ",
            "                        ",
        },
    };

    for (uint32_t f = 0; f < 8; f++) {
        uint32_t* bmp = sb->bitmaps[f];
        for (uint32_t i = 0; i < 24 * 24; i++) bmp[i] = 0;
        for (uint32_t y = 0; y < 24; y++) {
            for (uint32_t x = 0; x < 24; x++) {
                char c = spinner_frames[f][y][x];
                uint32_t color = 0;
                if (c == 'B') color = 0xFF000000;
                else if (c == 'W') color = 0xFFCCCCCC; // Light grey
                else if (c == 'D') color = 0xFF404040; // Dark grey
                if (color != 0) set_pixel(bmp, 24, x, y, color);
            }
        }
    }
    /* Wait Hourglass (4 animated frames) */
    CursorThemeSprite* sw = &g_theme_sprites[CURSOR_SHAPE_WAIT];
    sw->width = 24;
    sw->height = 24;
    sw->hotspot_x = 12;
    sw->hotspot_y = 12;
    sw->frame_count = 4;
    sw->frame_interval_ms = 250;
    sw->is_animated = true;

    for (uint32_t f = 0; f < 4; f++) {
        uint32_t* bmp = sw->bitmaps[f];
        for (uint32_t i = 0; i < sw->width * sw->height; i++) bmp[i] = 0x00000000;
        draw_hline(bmp, sw->width, 8, 16, 6, 0xFFFFFFFF);
        draw_hline(bmp, sw->width, 8, 16, 18, 0xFFFFFFFF);
        draw_vline(bmp, sw->width, 8, 6, 18, 0xFFFFFFFF);
        draw_vline(bmp, sw->width, 16, 6, 18, 0xFFFFFFFF);
        /* Sand level animation inside hourglass */
        uint32_t sand = 0xFF00AAFF;
        if (f == 0) draw_hline(bmp, sw->width, 9, 15, 8, sand);
        if (f == 1) draw_hline(bmp, sw->width, 10, 14, 11, sand);
        if (f == 2) draw_hline(bmp, sw->width, 10, 14, 13, sand);
        if (f == 3) draw_hline(bmp, sw->width, 9, 15, 16, sand);
    }
}

static void init_crosshair_and_hand(void) {
    /* Crosshair */
    CursorThemeSprite* sc = &g_theme_sprites[CURSOR_SHAPE_CROSSHAIR];
    sc->width = 24;
    sc->height = 24;
    sc->hotspot_x = 12;
    sc->hotspot_y = 12;
    sc->frame_count = 1;
    sc->is_animated = false;
    uint32_t* bmpc = sc->bitmaps[0];
    for (uint32_t i = 0; i < sc->width * sc->height; i++) bmpc[i] = 0x00000000;
    draw_hline(bmpc, sc->width, 4, 20, 12, 0xFFFFFFFF);
    draw_vline(bmpc, sc->width, 12, 4, 20, 0xFFFFFFFF);
    set_pixel(bmpc, sc->width, 12, 12, 0xFF000000); /* Center dot */

    /* Pointing Hand */
    CursorThemeSprite* sh = &g_theme_sprites[CURSOR_SHAPE_HAND];
    sh->width = 20;
    sh->height = 24;
    sh->hotspot_x = 8;
    sh->hotspot_y = 2;
    sh->frame_count = 1;
    sh->is_animated = false;
    uint32_t* bmph = sh->bitmaps[0];
    for (uint32_t i = 0; i < sh->width * sh->height; i++) bmph[i] = 0x00000000;
    /* Pointing index finger */
    draw_vline(bmph, sh->width, 8, 2, 12, 0xFFFFFFFF);
    draw_vline(bmph, sh->width, 7, 2, 12, 0xFF000000);
    draw_vline(bmph, sh->width, 9, 2, 12, 0xFF000000);
    /* Hand palm */
    for (uint32_t y = 10; y < 20; y++) {
        draw_hline(bmph, sh->width, 6, 14, y, 0xFFFFFFFF);
    }
    draw_hline(bmph, sh->width, 6, 14, 9, 0xFF000000);
    draw_hline(bmph, sh->width, 6, 14, 20, 0xFF000000);
}

void cursor_theme_init(void) {
    init_arrow_sprite();
    init_text_beam_sprite();
    init_resize_sprites();
    init_animated_sprites();
    init_crosshair_and_hand();
}

const uint32_t* cursor_theme_get_bitmap(
    CursorShape shape,
    uint32_t anim_frame,
    uint32_t* out_w,
    uint32_t* out_h,
    uint32_t* out_hx,
    uint32_t* out_hy
) {
    if (shape >= CURSOR_SHAPE_MAX) shape = CURSOR_SHAPE_ARROW;
    CursorThemeSprite* s = &g_theme_sprites[shape];

    if (s->frame_count == 0) s->frame_count = 1;
    uint32_t f = anim_frame % s->frame_count;

    if (out_w) *out_w = s->width;
    if (out_h) *out_h = s->height;
    if (out_hx) *out_hx = s->hotspot_x;
    if (out_hy) *out_hy = s->hotspot_y;

    return s->bitmaps[f];
}

bool cursor_theme_is_animated(CursorShape shape, uint32_t* out_frame_count, uint32_t* out_interval_ms) {
    if (shape >= CURSOR_SHAPE_MAX) return false;
    CursorThemeSprite* s = &g_theme_sprites[shape];
    if (out_frame_count) *out_frame_count = s->frame_count;
    if (out_interval_ms) *out_interval_ms = s->frame_interval_ms;
    return s->is_animated;
}
