#include "../Include/text.h"
#include "../Include/graphics.h"
#include "kernel/ui/bofont/bofont.h"
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
    (void)font;
    BVTextMetrics result = {0, 0, 0, 0, 0};
    if (!str || !*str) return result;

    BOTextMetrics tm = BOFont_MeasureTextRole(BOFONT_ROLE_MONO, str);
    result.width = tm.width;
    result.height = tm.height;
    result.line_height = 16;
    result.baseline = 13;
    result.char_count = tm.char_count;
    return result;
}

void BOVISUAL_Draw_Char(int32_t x, int32_t y, char c, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font) {
    (void)bg_color;
    (void)transparent_bg;
    (void)font;
    if (!g_text_ready) return;

    char str[2] = {c, '\0'};
    BOFont_DrawTextRole(BOFONT_ROLE_MONO, str, x, y, fg_color);
}

void BOVISUAL_Draw_String(int32_t x, int32_t y, const char* str, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font) {
    (void)bg_color;
    (void)transparent_bg;
    (void)font;
    if (!str) return;

    BOFont_DrawTextRole(BOFONT_ROLE_MONO, str, x, y, fg_color);
}
