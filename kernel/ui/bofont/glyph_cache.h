#ifndef KERNEL_BOFONT_GLYPH_CACHE_H
#define KERNEL_BOFONT_GLYPH_CACHE_H

#include "font_types.h"

// Glyph Cache Subsystem
// Enforces strict rules:
// - Never generate or rasterize glyphs during rendering.
// - Once a glyph is generated and stored in the atlas, it remains cached forever.
// - Duplicate generation is strictly forbidden.
// - Missing codepoints gracefully fall back to a replacement glyph without errors.

void BOGlyphCache_Init(void);

// Retrieve cached glyph metadata for a given codepoint.
// Returns a valid pointer to either the requested glyph or the replacement glyph (codepoint 127).
const BOGlyph* BOGlyphCache_GetGlyph(BOFont* font, uint32_t codepoint);

// Get diagnostic usage statistics
void BOGlyphCache_GetStats(uint32_t* out_hits, uint32_t* out_misses, uint32_t* out_total_cached);

#endif // KERNEL_BOFONT_GLYPH_CACHE_H
