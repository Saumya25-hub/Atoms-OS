#ifndef KERNEL_BOFONT_TEXT_LAYOUT_H
#define KERNEL_BOFONT_TEXT_LAYOUT_H

#include "font_types.h"
#include "glyph_cache.h"

// Text Layout Engine Subsystem
// Responsible solely for calculating exact character positioning, line breaks,
// tabs, new lines, cursor advance, horizontal/vertical spacing, and alignment.
// Does NOT perform any pixel rendering or framebuffer access.

// Callback signature invoked for each calculated glyph layout run
typedef void (*BOLayoutCallback)(const BOLayoutGlyph* glyph_item, void* user_data);

// Initialize text layout subsystem
void BOTextLayout_Init(void);

// Measure exact bounding dimensions and line count of unconstrained string
BOTextMetrics BOTextLayout_Measure(BOFont* font, const char* text);

// Measure bounding dimensions constrained by maximum width and flags (word wrap, etc.)
BOTextMetrics BOTextLayout_MeasureEx(BOFont* font, const char* text, int32_t max_width, uint32_t flags);

// Calculate layout run and invoke callback for each positioned glyph
void BOTextLayout_Run(BOFont* font, const char* text, int32_t start_x, int32_t start_y, uint32_t color, BOLayoutCallback callback, void* user_data);

// Calculate constrained layout run with word wrapping and alignment flags
void BOTextLayout_RunEx(BOFont* font, const char* text, int32_t start_x, int32_t start_y, int32_t max_width, uint32_t color, uint32_t flags, BOLayoutCallback callback, void* user_data);

#endif // KERNEL_BOFONT_TEXT_LAYOUT_H
