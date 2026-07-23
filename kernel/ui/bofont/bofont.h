#ifndef KERNEL_BOFONT_BOFONT_H
#define KERNEL_BOFONT_BOFONT_H

#include "font_types.h"
#include "font_loader.h"
#include "glyph_cache.h"
#include "glyph_atlas.h"
#include "text_layout.h"

// BOFONT ENGINE v3 Master API
// The official typography subsystem for ATOMS OS.
// Responsible for font lifecycle, glyph caching, atlas generation, layout,
// system font role management, and batched submission to BOIMAGE / BOHEART.

// Initialize the typography engine and build default system font atlases for all roles
void BOFont_Initialize(void);

// Shutdown the engine and release all font assets
void BOFont_Shutdown(void);

// Get font object for a semantic system role
BOFont* BOFont_GetRole(BOFontRole role);

// Get the default built-in system font (UI_REGULAR or fallback)
BOFont* BOFont_GetDefault(void);

// Load a font asset object into a new BOFont structure
BOFont* BOFont_LoadAsset(uint32_t font_id, BOFontRole role, const BOFontAsset* asset);

// Load a binary BMF / bitmap font buffer into a new BOFont structure
BOFont* BOFont_Load(uint32_t font_id, const char* name, const uint8_t* font_data, uint32_t data_size);

// Load an embedded bitmap table into a new BOFont structure
BOFont* BOFont_LoadEmbedded(uint32_t font_id, const char* name, const uint8_t* bitmap_data, int32_t char_w, int32_t char_h);

// Unload and destroy a font
void BOFont_Unload(BOFont* font);

// Retrieve cached glyph metadata
const BOGlyph* BOFont_GetGlyph(BOFont* font, uint32_t codepoint);

// Measure bounding dimensions of unconstrained text
BOTextMetrics BOFont_MeasureText(BOFont* font, const char* text);
BOTextMetrics BOFont_MeasureTextRole(BOFontRole role, const char* text);

#include "bovisual/Include/bovisual_types.h"

// Draw text string using the specified font via BOIMAGE batch renderer
void BOFont_DrawText(BOFont* font, const char* text, int32_t x, int32_t y, uint32_t color);
void BOFont_DrawTextRole(BOFontRole role, const char* text, int32_t x, int32_t y, uint32_t color);
void BOFont_DrawTextTarget(const BVFramebuffer* target_fb, BOFont* font, const char* text, int32_t x, int32_t y, uint32_t color);
void BOFont_DrawTextRoleTarget(const BVFramebuffer* target_fb, BOFontRole role, const char* text, int32_t x, int32_t y, uint32_t color);

// Draw text string with layout constraints (word wrap, alignment, max width)
void BOFont_DrawTextEx(BOFont* font, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags);
void BOFont_DrawTextRoleEx(BOFontRole role, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags);
void BOFont_DrawTextTargetEx(const BVFramebuffer* target_fb, BOFont* font, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags);
void BOFont_DrawTextRoleTargetEx(const BVFramebuffer* target_fb, BOFontRole role, const char* text, int32_t x, int32_t y, int32_t max_width, uint32_t color, uint32_t flags);

// Debugging Overlay APIs
void BOFont_SetDebugOverlay(bool enabled);
bool BOFont_IsDebugOverlayEnabled(void);
void BOFont_DrawDebugOverlay(int32_t screen_x, int32_t screen_y);

#endif // KERNEL_BOFONT_BOFONT_H
