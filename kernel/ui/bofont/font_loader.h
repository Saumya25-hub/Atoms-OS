#ifndef KERNEL_BOFONT_FONT_LOADER_H
#define KERNEL_BOFONT_FONT_LOADER_H

#include "font_types.h"

// Font Loader Subsystem
// Responsible solely for parsing font headers, validating font data buffers,
// and populating basic font & glyph metadata into BOFont structures.
// Does NOT perform GPU atlas generation or rendering.

// Initialize font loader subsystem
void BOFontLoader_Init(void);

// Parse an embedded fixed-width bitmap table (e.g., 8x16 system font)
// Populates basic font metrics and initial glyph table dimensions.
int BOFontLoader_LoadEmbeddedBitmap(BOFont* font, uint32_t font_id, const char* name, const uint8_t* raw_bitmap_table, int32_t char_w, int32_t char_h);

// Parse binary BMF / bitmap font buffer loaded from storage
int BOFontLoader_LoadBMF(BOFont* font, uint32_t font_id, const char* name, const uint8_t* data, uint32_t size);

// Validate font data integrity safely without kernel panics
bool BOFontLoader_ValidateFontData(const uint8_t* data, uint32_t size);

#endif // KERNEL_BOFONT_FONT_LOADER_H
