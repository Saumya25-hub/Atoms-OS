#include "font_loader.h"
#include "kernel/core/lib/include/string.h"

void BOFontLoader_Init(void) {
    // Phase 1 font loader initialization
}

bool BOFontLoader_ValidateFontData(const uint8_t* data, uint32_t size) {
    if (!data || size == 0) return false;
    // For Phase 1 BMF / raw bitmap validation: ensure reasonable minimum header or data size
    if (size < 16) return false;
    return true;
}

int BOFontLoader_LoadEmbeddedBitmap(BOFont* font, uint32_t font_id, const char* name, const uint8_t* raw_bitmap_table, int32_t char_w, int32_t char_h) {
    if (!font || !raw_bitmap_table || char_w <= 0 || char_h <= 0) {
        return BOFONT_ERR_INVALID_PARAM;
    }

    memset(font, 0, sizeof(BOFont));
    font->font_id = font_id;
    if (name) {
        strncpy(font->name, name, sizeof(font->name) - 1);
        font->name[sizeof(font->name) - 1] = '\0';
    } else {
        strcpy(font->name, "EmbeddedBitmap");
    }

    font->glyph_size = char_h;
    font->line_height = char_h;
    font->spacing = 0;
    font->ascender = (char_h * 13) / 16;
    font->descender = char_h - font->ascender;
    font->atlas_texture = NULL;
    font->atlas_handle = NULL;
    font->is_loaded = false;
    font->ref_count = 1;

    for (int i = 0; i < BOFONT_MAX_ASCII_GLYPHS; i++) {
        BOGlyph* g = &font->glyph_table[i];
        g->codepoint = (uint32_t)i;
        g->width = char_w;
        g->height = char_h;
        g->advance_x = char_w;
        g->bearing_x = 0;
        g->bearing_y = font->ascender;
        g->atlas_x = 0;
        g->atlas_y = 0;
        g->u1 = 0.0f;
        g->v1 = 0.0f;
        g->u2 = 0.0f;
        g->v2 = 0.0f;
        g->cached = false;
    }

    return BOFONT_OK;
}

int BOFontLoader_LoadBMF(BOFont* font, uint32_t font_id, const char* name, const uint8_t* data, uint32_t size) {
    if (!font || !BOFontLoader_ValidateFontData(data, size)) {
        return BOFONT_ERR_INVALID_FONT_DATA;
    }

    // In Phase 1, BMF format parses simple bitmap headers. If header matches BMF magic:
    // If not standard BMF, fallback cleanly or reject safely without panic.
    if (data[0] == 'B' && data[1] == 'M' && data[2] == 'F') {
        int32_t char_w = data[4];
        int32_t char_h = data[5];
        if (char_w <= 0 || char_h <= 0) return BOFONT_ERR_INVALID_FONT_DATA;
        const uint8_t* bitmap_ptr = data + 16;
        return BOFontLoader_LoadEmbeddedBitmap(font, font_id, name, bitmap_ptr, char_w, char_h);
    }

    return BOFONT_ERR_INVALID_FONT_DATA;
}
