#include "../Include/text.h"
#include "../Include/graphics.h"

// Using the embedded 8x16 font generated
#include "font8x16.h"
static bool g_text_ready = false;

static BVFontMetrics g_default_font = {
    .glyph_width = 8,
    .glyph_height = 16,
    .advance_x = 8,
    .baseline = 13,
    .ascender = 13,
    .descender = 3,
    .line_height = 16
};

void BOVISUAL_Text_Init(void) {
    g_text_ready = true;
}

const BVFontMetrics* BV_GetDefaultFont(void) {
    return &g_default_font;
}

BVTextMetrics BV_TextMeasure(const BVFontMetrics* font, const char* str) {
    BVTextMetrics result = {0, 0, 0, 0, 0};
    if (!font) return result;
    
    result.baseline = font->baseline;
    result.line_height = font->line_height;

    if (!str || !*str) return result;

    int32_t current_w = 0;
    int32_t max_w = 0;
    int32_t lines = 1;

    while (*str) {
        if (*str == '\n') {
            if (current_w > max_w) max_w = current_w;
            current_w = 0;
            lines++;
        } else {
            current_w += font->advance_x;
            result.char_count++;
        }
        str++;
    }

    if (current_w > max_w) max_w = current_w;
    
    result.width = max_w;
    result.height = lines * font->line_height;
    return result;
}

void BOVISUAL_Draw_Char(int32_t x, int32_t y, char c, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font) {
    if (!g_text_ready || !font) return;

    // Hardcoded to 8x16 bitmap for now, but position uses metrics
    const uint8_t* glyph = g_font8x16_stub[(uint8_t)c];

    for (int32_t row = 0; row < 16; row++) {
        uint8_t row_data = glyph[row];
        for (int32_t col = 0; col < 8; col++) {
            bool pixel_on = (row_data & (1 << col)) != 0;
            if (pixel_on) {
                BOVISUAL_Graphics_PutPixel(x + col, y + row, fg_color);
            } else if (!transparent_bg) {
                BOVISUAL_Graphics_PutPixel(x + col, y + row, bg_color);
            }
        }
    }
}

void BOVISUAL_Draw_String(int32_t x, int32_t y, const char* str, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font) {
    if (!str || !font) return;

    int32_t cursor_x = x;
    while (*str) {
        if (*str == '\n') {
            y += font->line_height;
            cursor_x = x;
        } else {
            BOVISUAL_Draw_Char(cursor_x, y, *str, fg_color, bg_color, transparent_bg, font);
            cursor_x += font->advance_x;
        }
        str++;
    }
}
