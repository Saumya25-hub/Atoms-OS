#ifndef KERNEL_BOFONT_FONT_TYPES_H
#define KERNEL_BOFONT_FONT_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/ui/boimage/boimage.h"

// Error codes
#define BOFONT_OK                      0
#define BOFONT_ERR_INVALID_PARAM      -1
#define BOFONT_ERR_OUT_OF_MEMORY      -2
#define BOFONT_ERR_ATLAS_FULL         -3
#define BOFONT_ERR_GLYPH_MISSING      -4
#define BOFONT_ERR_INVALID_FONT_DATA  -5
#define BOFONT_ERR_NOT_INITIALIZED    -6

// Glyph Object
// Represents a single character/codepoint inside the typography engine.
typedef struct BOGlyph {
    uint32_t codepoint;     // Future Unicode UTF-32 ready (Phase 1 uses ASCII 32..126)
    int32_t atlas_x;        // Pixel coordinate X inside the glyph atlas
    int32_t atlas_y;        // Pixel coordinate Y inside the glyph atlas
    int32_t width;          // Glyph bounding box width in pixels
    int32_t height;         // Glyph bounding box height in pixels
    int32_t advance_x;      // Horizontal cursor advance after drawing this glyph
    int32_t bearing_x;      // Horizontal offset from baseline origin to left of glyph
    int32_t bearing_y;      // Vertical offset from baseline origin to top of glyph
    float u1, v1;           // Normalized texture atlas UV coordinate (top-left)
    float u2, v2;           // Normalized texture atlas UV coordinate (bottom-right)
    bool cached;            // True if this glyph has been generated and cached
} BOGlyph;

// Font Object
// Contains all typography properties and holds reference to the generated glyph atlas.
#define BOFONT_MAX_ASCII_GLYPHS 128

typedef struct BOFont {
    uint32_t font_id;               // Unique identifier for font registry
    char name[64];                  // Readable name (e.g. "System-8x16")
    int32_t glyph_size;             // Nominal font size in pixels
    int32_t line_height;            // Vertical advance per line (e.g. 16)
    int32_t spacing;                // Extra horizontal spacing between characters
    int32_t ascender;               // Distance from baseline to highest ascender
    int32_t descender;              // Distance from baseline to lowest descender
    BOTexture* atlas_texture;       // Backing texture atlas for BOIMAGE integration
    BOAtlas* atlas_handle;          // BOIMAGE cell-based atlas handle
    BOGlyph glyph_table[BOFONT_MAX_ASCII_GLYPHS]; // Phase 1 ASCII glyph lookup table
    bool is_loaded;                 // True when font data has been parsed and atlas built
    uint32_t ref_count;             // Reference count for shared font handles
} BOFont;

// Text Measurement Output
typedef struct BOTextMetrics {
    int32_t width;                  // Total bounding width of measured text
    int32_t height;                 // Total bounding height of measured text
    int32_t line_count;             // Number of text lines
    int32_t char_count;             // Total number of printable characters
    int32_t baseline;               // Vertical baseline offset
} BOTextMetrics;

// Text Layout Item
// Represents a single positioned glyph ready for submission to BOIMAGE batch queue.
typedef struct BOLayoutGlyph {
    const BOGlyph* glyph;           // Pointer to cached glyph object
    int32_t screen_x;               // Calculated absolute screen coordinate X
    int32_t screen_y;               // Calculated absolute screen coordinate Y
    uint32_t color;                 // ARGB color for sprite tinting
} BOLayoutGlyph;

// Text Draw Flags
#define BOFONT_FLAG_ALIGN_LEFT   (0 << 0)
#define BOFONT_FLAG_ALIGN_CENTER (1 << 0)
#define BOFONT_FLAG_ALIGN_RIGHT  (1 << 1)
#define BOFONT_FLAG_WORD_WRAP    (1 << 2)

#endif // KERNEL_BOFONT_FONT_TYPES_H
