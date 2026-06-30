#include "glyph_cache.h"

static uint32_t s_cache_hits = 0;
static uint32_t s_cache_misses = 0;

void BOGlyphCache_Init(void) {
    s_cache_hits = 0;
    s_cache_misses = 0;
}

const BOGlyph* BOGlyphCache_GetGlyph(BOFont* font, uint32_t codepoint) {
    if (!font || !font->is_loaded) {
        s_cache_misses++;
        return NULL;
    }

    // In Phase 1 (ASCII 0..127), codepoints outside this range fall back to replacement box (127)
    if (codepoint >= BOFONT_MAX_ASCII_GLYPHS) {
        s_cache_misses++;
        return &font->glyph_table[127]; // Replacement box
    }

    // Unprintable characters below 32 fall back to replacement box or space
    if (codepoint < 32 && codepoint != '\t' && codepoint != '\n' && codepoint != '\r') {
        s_cache_misses++;
        return &font->glyph_table[127];
    }

    const BOGlyph* g = &font->glyph_table[codepoint];
    if (g->cached) {
        s_cache_hits++;
        return g;
    }

    s_cache_misses++;
    return &font->glyph_table[127];
}

void BOGlyphCache_GetStats(uint32_t* out_hits, uint32_t* out_misses, uint32_t* out_total_cached) {
    if (out_hits) *out_hits = s_cache_hits;
    if (out_misses) *out_misses = s_cache_misses;
    if (out_total_cached) *out_total_cached = BOFONT_MAX_ASCII_GLYPHS;
}
