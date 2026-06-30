#include "bofont.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/string.h"
#include "bovisual/Text/font8x16.h"

static bool s_bofont_initialized = false;
static BOFont s_default_font_storage;
static bool s_debug_overlay_enabled = false;

static uint32_t s_loaded_fonts_count = 0;
static uint32_t s_total_draw_calls = 0;
static uint32_t s_total_glyphs_submitted = 0;

static void bofont_layout_callback(const BOLayoutGlyph* item, void* user_data) {
    BOFont* font = (BOFont*)user_data;
    if (!font || !font->atlas_texture || !item->glyph) return;
    const BOGlyph* g = item->glyph;
    BOImage_BatchDrawSpriteTinted(font->atlas_texture, item->screen_x, item->screen_y, g->width, g->height, g->u1, g->v1, g->u2, g->v2, item->color);
    s_total_glyphs_submitted++;
}

void BOFont_Initialize(void) {
    if (s_bofont_initialized) return;

    BOFontLoader_Init();
    BOGlyphCache_Init();
    BOTextLayout_Init();

    // Load and build the default system font (8x16 bitmap)
    BOFontLoader_LoadEmbeddedBitmap(&s_default_font_storage, 1, "System-8x16", (const uint8_t*)g_font8x16_stub, 8, 16);
    BOFontAtlas_Build(&s_default_font_storage, (const uint8_t*)g_font8x16_stub);

    s_loaded_fonts_count = 1;
    s_bofont_initialized = true;
}

void BOFont_Shutdown(void) {
    if (!s_bofont_initialized) return;

    BOFontAtlas_Destroy(&s_default_font_storage);
    s_loaded_fonts_count = 0;
    s_bofont_initialized = false;
}

BOFont* BOFont_GetDefault(void) {
    if (!s_bofont_initialized) {
        BOFont_Initialize();
    }
    return &s_default_font_storage;
}

BOFont* BOFont_Load(uint32_t font_id, const char* name, const uint8_t* font_data, uint32_t data_size) {
    if (!s_bofont_initialized || !font_data || data_size == 0) return NULL;

    BOFont* font = (BOFont*)kmalloc(sizeof(BOFont));
    if (!font) return NULL;

    int res = BOFontLoader_LoadBMF(font, font_id, name, font_data, data_size);
    if (res != BOFONT_OK) {
        kfree(font);
        return NULL;
    }

    // Notice that if BMF was loaded as bitmap table, build the atlas
    // If bitmap pointer was extracted in LoadBMF, build atlas:
    const uint8_t* bitmap_ptr = font_data;
    if (font_data[0] == 'B' && font_data[1] == 'M' && font_data[2] == 'F') {
        bitmap_ptr = font_data + 16;
    }
    res = BOFontAtlas_Build(font, bitmap_ptr);
    if (res != BOFONT_OK) {
        kfree(font);
        return NULL;
    }

    s_loaded_fonts_count++;
    return font;
}

BOFont* BOFont_LoadEmbedded(uint32_t font_id, const char* name, const uint8_t* bitmap_data, int32_t char_w, int32_t char_h) {
    if (!s_bofont_initialized || !bitmap_data) return NULL;

    BOFont* font = (BOFont*)kmalloc(sizeof(BOFont));
    if (!font) return NULL;

    int res = BOFontLoader_LoadEmbeddedBitmap(font, font_id, name, bitmap_data, char_w, char_h);
    if (res != BOFONT_OK) {
        kfree(font);
        return NULL;
    }

    res = BOFontAtlas_Build(font, bitmap_data);
    if (res != BOFONT_OK) {
        kfree(font);
        return NULL;
    }

    s_loaded_fonts_count++;
    return font;
}

void BOFont_Unload(BOFont* font) {
    if (!font || font == &s_default_font_storage) return;

    if (font->ref_count > 1) {
        font->ref_count--;
        return;
    }

    BOFontAtlas_Destroy(font);
    if (s_loaded_fonts_count > 0) s_loaded_fonts_count--;
    kfree(font);
}

const BOGlyph* BOFont_GetGlyph(BOFont* font, uint32_t codepoint) {
    if (!font) font = BOFont_GetDefault();
    return BOGlyphCache_GetGlyph(font, codepoint);
}

BOTextMetrics BOFont_MeasureText(BOFont* font, const char* text) {
    if (!font) font = BOFont_GetDefault();
    return BOTextLayout_Measure(font, text);
}

void BOFont_DrawText(BOFont* font, const char* text, int32_t x, int32_t y, uint32_t color) {
    BOFont_DrawTextEx(font, text, x, y, 0, color, 0);
}

void BOFont_DrawTextEx(BOFont* font, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags) {
    if (!s_bofont_initialized) BOFont_Initialize();
    if (!font) font = BOFont_GetDefault();
    if (!text || !*text) return;

    s_total_draw_calls++;
    BOTextLayout_RunEx(font, text, x, y, max_width, color, flags, bofont_layout_callback, font);
}

void BOFont_SetDebugOverlay(bool enabled) {
    s_debug_overlay_enabled = enabled;
}

bool BOFont_IsDebugOverlayEnabled(void) {
    return s_debug_overlay_enabled;
}

static void bofont_utoa(uint32_t val, char* buf) {
    char tmp[32];
    int idx = 0;
    if (val == 0) {
        tmp[idx++] = '0';
    } else {
        while (val > 0) {
            tmp[idx++] = '0' + (val % 10);
            val /= 10;
        }
    }
    int out = 0;
    while (idx > 0) {
        buf[out++] = tmp[--idx];
    }
    buf[out] = '\0';
}

void BOFont_DrawDebugOverlay(int32_t screen_x, int32_t screen_y) {
    if (!s_debug_overlay_enabled) return;

    BOFont* font = BOFont_GetDefault();
    if (!font) return;

    uint32_t hits = 0, misses = 0, cached = 0;
    BOGlyphCache_GetStats(&hits, &misses, &cached);

    char num_buf[32];
    char line[64];

    // Background panel could be drawn by surface or simply draw text directly
    int32_t y = screen_y;
    BOFont_DrawText(font, "=== BOFONT v2 DEBUG OVERLAY ===", screen_x, y, 0xFF00FF00);
    y += font->line_height;

    strcpy(line, "Fonts Loaded: ");
    bofont_utoa(s_loaded_fonts_count, num_buf);
    strcat(line, num_buf);
    BOFont_DrawText(font, line, screen_x, y, 0xFFFFFFFF);
    y += font->line_height;

    strcpy(line, "Cached Glyphs: ");
    bofont_utoa(cached, num_buf);
    strcat(line, num_buf);
    BOFont_DrawText(font, line, screen_x, y, 0xFFFFFFFF);
    y += font->line_height;

    strcpy(line, "Cache Hits: ");
    bofont_utoa(hits, num_buf);
    strcat(line, num_buf);
    BOFont_DrawText(font, line, screen_x, y, 0xFFFFFFFF);
    y += font->line_height;

    strcpy(line, "Text Draw Calls: ");
    bofont_utoa(s_total_draw_calls, num_buf);
    strcat(line, num_buf);
    BOFont_DrawText(font, line, screen_x, y, 0xFFFFFFFF);
    y += font->line_height;

    strcpy(line, "Glyphs Submitted: ");
    bofont_utoa(s_total_glyphs_submitted, num_buf);
    strcat(line, num_buf);
    BOFont_DrawText(font, line, screen_x, y, 0xFFFFFFFF);
}
