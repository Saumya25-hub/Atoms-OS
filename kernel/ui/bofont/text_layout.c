#include "text_layout.h"

void BOTextLayout_Init(void) {
    // Layout engine initialization
}

BOTextMetrics BOTextLayout_Measure(BOFont* font, const char* text) {
    return BOTextLayout_MeasureEx(font, text, 0, 0);
}

BOTextMetrics BOTextLayout_MeasureEx(BOFont* font, const char* text, int32_t max_width, uint32_t flags) {
    BOTextMetrics metrics = {0, 0, 0, 0, 0};
    if (!font || !text || !*text) return metrics;

    metrics.baseline = font->ascender;
    metrics.line_count = 1;

    int32_t cur_x = 0;
    int32_t max_x = 0;
    int32_t space_adv = font->glyph_table[' '].advance_x;
    if (space_adv <= 0) space_adv = 8;
    int32_t tab_adv = space_adv * 4;

    while (*text) {
        char c = *text++;
        if (c == '\n') {
            if (cur_x > max_x) max_x = cur_x;
            cur_x = 0;
            metrics.line_count++;
            continue;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\t') {
            int32_t next_tab = ((cur_x / tab_adv) + 1) * tab_adv;
            cur_x = next_tab;
            metrics.char_count++;
            continue;
        }

        const BOGlyph* g = BOGlyphCache_GetGlyph(font, (uint32_t)(uint8_t)c);
        if (!g) continue;

        if ((flags & BOFONT_FLAG_WORD_WRAP) && max_width > 0) {
            if (cur_x + g->advance_x > max_width && cur_x > 0) {
                if (cur_x > max_x) max_x = cur_x;
                cur_x = 0;
                metrics.line_count++;
            }
        }

        cur_x += g->advance_x + font->spacing;
        metrics.char_count++;
    }

    if (cur_x > max_x) max_x = cur_x;
    metrics.width = max_x;
    metrics.height = metrics.line_count * font->line_height;
    return metrics;
}

void BOTextLayout_Run(BOFont* font, const char* text, int32_t start_x, int32_t start_y, uint32_t color, BOLayoutCallback callback, void* user_data) {
    BOTextLayout_RunEx(font, text, start_x, start_y, 0, color, 0, callback, user_data);
}

void BOTextLayout_RunEx(BOFont* font, const char* text, int32_t start_x, int32_t start_y, int32_t max_width, uint32_t color, uint32_t flags, BOLayoutCallback callback, void* user_data) {
    if (!font || !text || !callback) return;

    int32_t cur_x = start_x;
    int32_t cur_y = start_y;
    int32_t space_adv = font->glyph_table[' '].advance_x;
    if (space_adv <= 0) space_adv = 8;
    int32_t tab_adv = space_adv * 4;

    while (*text) {
        char c = *text++;
        if (c == '\n') {
            cur_y += font->line_height;
            cur_x = start_x;
            continue;
        }
        if (c == '\r') {
            cur_x = start_x;
            continue;
        }
        if (c == '\t') {
            int32_t offset = cur_x - start_x;
            int32_t next_tab = ((offset / tab_adv) + 1) * tab_adv;
            cur_x = start_x + next_tab;
            continue;
        }

        const BOGlyph* g = BOGlyphCache_GetGlyph(font, (uint32_t)(uint8_t)c);
        if (!g) continue;

        if ((flags & BOFONT_FLAG_WORD_WRAP) && max_width > 0) {
            if (cur_x - start_x + g->advance_x > max_width && cur_x > start_x) {
                cur_y += font->line_height;
                cur_x = start_x;
            }
        }

        BOLayoutGlyph item;
        item.glyph = g;
        item.screen_x = cur_x + g->bearing_x;
        item.screen_y = cur_y + g->bearing_y;
        item.color = color;

        callback(&item, user_data);

        cur_x += g->advance_x + font->spacing;
    }
}
