#include "bofont.h"
#include "bofont_assets.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "bovisual/Text/font8x16.h"

static bool s_bofont_initialized = false;
static BOFont s_role_fonts[BOFONT_ROLE_COUNT];
static BOFont s_fallback_font_storage;
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

static void bofont_init_fallback(void) {
    BOFontLoader_LoadEmbeddedBitmap(&s_fallback_font_storage, 999, "Emergency-Fallback-8x16", (const uint8_t*)g_font8x16_stub, 8, 16);
    BOFontAtlas_Build(&s_fallback_font_storage, (const uint8_t*)g_font8x16_stub);
    s_fallback_font_storage.role = BOFONT_ROLE_UI_REGULAR;
}

void BOFont_Initialize(void) {
    if (s_bofont_initialized) return;

    BOFontLoader_Init();
    BOGlyphCache_Init();
    BOTextLayout_Init();

    // 1. Build Emergency Fallback Font first
    bofont_init_fallback();

    // 2. Load system font roles from pre-compiled assets
    const BOFontAsset* role_assets[BOFONT_ROLE_COUNT] = {
        [BOFONT_ROLE_UI_REGULAR] = &g_bofont_asset_ui_regular,
        [BOFONT_ROLE_UI_MEDIUM]  = &g_bofont_asset_ui_medium,
        [BOFONT_ROLE_UI_BOLD]    = &g_bofont_asset_ui_bold,
        [BOFONT_ROLE_CAPTION]    = &g_bofont_asset_caption,
        [BOFONT_ROLE_TITLE]      = &g_bofont_asset_title,
        [BOFONT_ROLE_MONO]       = &g_bofont_asset_mono,
    };

    s_loaded_fonts_count = 0;
    for (int r = 0; r < BOFONT_ROLE_COUNT; r++) {
        BOFont* font = &s_role_fonts[r];
        memset(font, 0, sizeof(BOFont));
        font->font_id = r + 1;
        font->role = (BOFontRole)r;
        font->ref_count = 1;

        int res = BOFontAtlas_BuildFromAsset(font, role_assets[r]);
        if (res == BOFONT_OK && font->is_loaded) {
            s_loaded_fonts_count++;
        } else {
            // Asset failed to build atlas: fallback gracefully to emergency 8x16 font
            memcpy(font, &s_fallback_font_storage, sizeof(BOFont));
            font->role = (BOFontRole)r;
        }
    }

    s_bofont_initialized = true;
}

void BOFont_Shutdown(void) {
    if (!s_bofont_initialized) return;

    for (int r = 0; r < BOFONT_ROLE_COUNT; r++) {
        BOFontAtlas_Destroy(&s_role_fonts[r]);
    }
    BOFontAtlas_Destroy(&s_fallback_font_storage);

    s_loaded_fonts_count = 0;
    s_bofont_initialized = false;
}

BOFont* BOFont_GetRole(BOFontRole role) {
    if (!s_bofont_initialized) {
        BOFont_Initialize();
    }
    if ((int)role < 0 || (int)role >= BOFONT_ROLE_COUNT) {
        return &s_role_fonts[BOFONT_ROLE_UI_REGULAR];
    }
    BOFont* font = &s_role_fonts[role];
    if (!font->is_loaded) {
        return &s_fallback_font_storage;
    }
    return font;
}

BOFont* BOFont_GetDefault(void) {
    return BOFont_GetRole(BOFONT_ROLE_UI_REGULAR);
}

BOFont* BOFont_LoadAsset(uint32_t font_id, BOFontRole role, const BOFontAsset* asset) {
    if (!s_bofont_initialized || !asset) return NULL;

    BOFont* font = (BOFont*)kmalloc(sizeof(BOFont));
    if (!font) return NULL;

    memset(font, 0, sizeof(BOFont));
    font->font_id = font_id;
    font->role = role;
    font->ref_count = 1;

    int res = BOFontAtlas_BuildFromAsset(font, asset);
    if (res != BOFONT_OK) {
        kfree(font);
        return NULL;
    }

    s_loaded_fonts_count++;
    return font;
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
    if (!font) return;
    for (int r = 0; r < BOFONT_ROLE_COUNT; r++) {
        if (font == &s_role_fonts[r]) return; // Never unload static role storage
    }
    if (font == &s_fallback_font_storage) return;

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

BOTextMetrics BOFont_MeasureTextRole(BOFontRole role, const char* text) {
    BOFont* font = BOFont_GetRole(role);
    return BOTextLayout_Measure(font, text);
}

typedef struct {
    BOFont* font;
    const BVFramebuffer* target_fb;
} BOFontRenderContext;

static void bofont_layout_callback_target(const BOLayoutGlyph* item, void* user_data) {
    BOFontRenderContext* ctx = (BOFontRenderContext*)user_data;
    if (!ctx || !ctx->font || !ctx->font->atlas_texture || !item->glyph) return;
    const BOGlyph* g = item->glyph;
    if (ctx->target_fb && ctx->target_fb->buffer) {
        extern void BOImage_DrawGlyphSpriteDirect(const BVFramebuffer* target_fb, BOTexture* texture,
                                           int32_t x, int32_t y, int32_t width, int32_t height,
                                           float u1, float v1, float u2, float v2,
                                           uint32_t tint_color);
        BOImage_DrawGlyphSpriteDirect(ctx->target_fb, ctx->font->atlas_texture,
                                      item->screen_x, item->screen_y,
                                      g->width, g->height,
                                      g->u1, g->v1, g->u2, g->v2,
                                      item->color);
    } else {
        BOImage_BatchDrawSpriteTinted(ctx->font->atlas_texture, item->screen_x, item->screen_y, g->width, g->height, g->u1, g->v1, g->u2, g->v2, item->color);
    }
    s_total_glyphs_submitted++;
}

void BOFont_DrawText(BOFont* font, const char* text, int32_t x, int32_t y, uint32_t color) {
    BOFont_DrawTextTargetEx(NULL, font, text, x, y, 0, color, 0);
}

void BOFont_DrawTextRole(BOFontRole role, const char* text, int32_t x, int32_t y, uint32_t color) {
    BOFont* font = BOFont_GetRole(role);
    BOFont_DrawTextTargetEx(NULL, font, text, x, y, 0, color, 0);
}

void BOFont_DrawTextTarget(const BVFramebuffer* target_fb, BOFont* font, const char* text, int32_t x, int32_t y, uint32_t color) {
    BOFont_DrawTextTargetEx(target_fb, font, text, x, y, 0, color, 0);
}

void BOFont_DrawTextRoleTarget(const BVFramebuffer* target_fb, BOFontRole role, const char* text, int32_t x, int32_t y, uint32_t color) {
    BOFont* font = BOFont_GetRole(role);
    BOFont_DrawTextTargetEx(target_fb, font, text, x, y, 0, color, 0);
}

void BOFont_DrawTextEx(BOFont* font, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags) {
    BOFont_DrawTextTargetEx(NULL, font, text, x, y, max_width, color, flags);
}

void BOFont_DrawTextRoleEx(BOFontRole role, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags) {
    BOFont* font = BOFont_GetRole(role);
    BOFont_DrawTextTargetEx(NULL, font, text, x, y, max_width, color, flags);
}

void BOFont_DrawTextTargetEx(const BVFramebuffer* target_fb, BOFont* font, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags) {
    if (!s_bofont_initialized) BOFont_Initialize();
    if (!font) font = BOFont_GetDefault();
    if (!text || !*text) return;

    s_total_draw_calls++;
    BOFontRenderContext ctx = { .font = font, .target_fb = target_fb };
    BOTextLayout_RunEx(font, text, x, y, max_width, color, flags, bofont_layout_callback_target, &ctx);
}

void BOFont_DrawTextRoleTargetEx(const BVFramebuffer* target_fb, BOFontRole role, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags) {
    BOFont* font = BOFont_GetRole(role);
    BOFont_DrawTextTargetEx(target_fb, font, text, x, y, max_width, color, flags);
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

    BOFont* font = BOFont_GetRole(BOFONT_ROLE_MONO);
    if (!font) return;

    uint32_t hits = 0, misses = 0, cached = 0;
    BOGlyphCache_GetStats(&hits, &misses, &cached);

    char num_buf[32];
    char line[64];

    int32_t y = screen_y;
    BOFont_DrawText(font, "=== BOFONT v3 DEBUG OVERLAY ===", screen_x, y, 0xFF00FF00);
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
