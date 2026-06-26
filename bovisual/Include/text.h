#ifndef BOVISUAL_TEXT_H
#define BOVISUAL_TEXT_H

#include "bovisual_types.h"

// Font Metrics
typedef struct {
    int32_t glyph_width;
    int32_t glyph_height;
    int32_t advance_x;
    int32_t baseline;
    int32_t ascender;
    int32_t descender;
    int32_t line_height;
} BVFontMetrics;

// Text Measurement Result
typedef struct {
    int32_t width;
    int32_t height;
    int32_t baseline;
    int32_t line_height;
    int32_t char_count;
} BVTextMetrics;

// Initialize text subsystem
void BOVISUAL_Text_Init(void);

// Get default system font
const BVFontMetrics* BV_GetDefaultFont(void);

// Measure text dimensions perfectly without drawing
BVTextMetrics BV_TextMeasure(const BVFontMetrics* font, const char* str);

// Draw a single character using exact font metrics
void BOVISUAL_Draw_Char(int32_t x, int32_t y, char c, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font);

// Draw a string using exact layout bounds
void BOVISUAL_Draw_String(int32_t x, int32_t y, const char* str, BOVISUAL_Color fg_color, BOVISUAL_Color bg_color, bool transparent_bg, const BVFontMetrics* font);

#endif // BOVISUAL_TEXT_H
