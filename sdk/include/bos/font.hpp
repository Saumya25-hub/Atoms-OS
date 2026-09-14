#ifndef BOS_UI_FONT_HPP
#define BOS_UI_FONT_HPP

#include <stdint.h>
#include <stddef.h>
#include "bos/types.hpp"
#include "bos/geometry.hpp"

namespace bos {

// ============================================================================
// Font Semantic Weights & Sizes
// ============================================================================
enum class FontWeight : uint8_t {
    Regular = 0,
    Medium  = 1,
    Bold    = 2
};

enum class FontSize : uint8_t {
    Caption    = 0, // 11px
    Body       = 1, // 13px
    Subheading = 2, // 15px
    Title      = 3  // 18px
};

enum class TextAlignment : uint8_t {
    Left   = 0,
    Center = 1,
    Right  = 2
};

// ============================================================================
// Text Measurement Metrics
// ============================================================================
struct TextMetrics {
    int32_t width{0};
    int32_t height{0};
    int32_t line_count{1};
    int32_t baseline{0};
    int32_t char_count{0};

    constexpr TextMetrics() = default;
    constexpr TextMetrics(int32_t w, int32_t h, int32_t lines, int32_t base, int32_t chars)
        : width(w), height(h), line_count(lines), baseline(base), char_count(chars) {}
};

// ============================================================================
// Compact Freestanding Glyph Data
// ============================================================================
struct GlyphData {
    uint8_t  codepoint;
    int8_t   advance_x;
    int8_t   bearing_x;
    int8_t   bearing_y;
    uint8_t  width;
    uint8_t  height;
    uint32_t alpha_offset;
};

// ============================================================================
// Immutable Font Asset (Compiled Static Glyph Atlas & Metrics)
// ============================================================================
struct FontAsset {
    const char*      family_name;
    FontWeight       weight;
    uint8_t          size_pt;
    uint8_t          line_height;
    int8_t           ascent;
    int8_t           descent;
    const GlyphData* glyphs;
    const uint8_t*   alpha_buffer;
    size_t           alpha_buffer_size;
};

// ============================================================================
// Font Handle & Measurement Engine
// ============================================================================
class Font {
public:
    constexpr Font() : m_asset(nullptr) {}
    constexpr explicit Font(const FontAsset* asset) : m_asset(asset) {}

    bool is_valid() const { return m_asset != nullptr; }
    const char* family() const { return m_asset ? m_asset->family_name : "Default"; }
    FontWeight weight() const { return m_asset ? m_asset->weight : FontWeight::Regular; }
    uint32_t size() const { return m_asset ? m_asset->size_pt : 13; }
    uint32_t line_height() const { return m_asset ? m_asset->line_height : 17; }
    int32_t ascent() const { return m_asset ? m_asset->ascent : 13; }
    int32_t descent() const { return m_asset ? m_asset->descent : 4; }
    const FontAsset* asset() const { return m_asset; }

    const GlyphData* get_glyph(char c) const;
    const uint8_t* get_glyph_bitmap(const GlyphData& g) const;

    TextMetrics measure(const char* text, int32_t max_width = -1) const;

    // Standard pre-configured font roles
    static const Font& Default();
    static const Font& Regular();
    static const Font& Bold();
    static const Font& Title();
    static const Font& Caption();

private:
    const FontAsset* m_asset{nullptr};
};

} // namespace bos

#endif // BOS_UI_FONT_HPP
