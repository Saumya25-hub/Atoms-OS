#ifndef KERNEL_BOFONT_GLYPH_ATLAS_H
#define KERNEL_BOFONT_GLYPH_ATLAS_H

#include "font_types.h"

// Glyph Atlas Subsystem
// Responsible for generating a single texture atlas containing every supported glyph,
// calculating exact normalized UV bounds for BOIMAGE batching, and managing texture lifecycles.

// Build a complete glyph texture atlas for the given font object using legacy 1-bit bitmap table data.
int BOFontAtlas_Build(BOFont* font, const uint8_t* raw_bitmap_table);

// Build a complete 8-bit Alpha coverage texture atlas for the given font object using native BOFontAsset.
int BOFontAtlas_BuildFromAsset(BOFont* font, const BOFontAsset* asset);

// Destroy and release memory associated with a font's glyph atlas.
void BOFontAtlas_Destroy(BOFont* font);

#endif // KERNEL_BOFONT_GLYPH_ATLAS_H
