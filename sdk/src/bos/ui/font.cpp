#include "bos/font.hpp"
#include "third_party/fonts/inter/inter_font_data.hpp"
#include <string.h>

namespace bos {

const GlyphData* Font::get_glyph(char c) const {
    if (!m_asset || !m_asset->glyphs) return nullptr;
    uint8_t uc = static_cast<uint8_t>(c);
    if (uc >= 32 && uc <= 126) {
        return &m_asset->glyphs[uc - 32];
    }
    // Fallback to '?' for unknown non-control characters
    if (uc > 126) {
        return &m_asset->glyphs['?' - 32];
    }
    return nullptr;
}

const uint8_t* Font::get_glyph_bitmap(const GlyphData& g) const {
    if (!m_asset || !m_asset->alpha_buffer) return nullptr;
    if (g.alpha_offset >= m_asset->alpha_buffer_size) return nullptr;
    return m_asset->alpha_buffer + g.alpha_offset;
}

TextMetrics Font::measure(const char* text, int32_t max_width) const {
    if (!text || text[0] == '\0' || !m_asset) {
        return TextMetrics(0, m_asset ? m_asset->line_height : 0, 1, m_asset ? m_asset->ascent : 0, 0);
    }

    int32_t max_w = 0;
    int32_t cur_w = 0;
    int32_t lines = 1;
    int32_t chars = 0;

    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c == '\n') {
            lines++;
            if (cur_w > max_w) max_w = cur_w;
            cur_w = 0;
            continue;
        }
        if (c == '\r') continue;

        const GlyphData* g = get_glyph(c);
        if (g) {
            cur_w += g->advance_x;
            chars++;
        }
        if (max_width > 0 && cur_w > max_width) {
            // Reached max_width boundary
            if (cur_w > max_w) max_w = cur_w;
            break;
        }
    }
    if (cur_w > max_w) max_w = cur_w;

    return TextMetrics(max_w, lines * m_asset->line_height, lines, m_asset->ascent, chars);
}

static const Font s_font_regular(&font_data::g_font_inter_regular_13);
static const Font s_font_bold(&font_data::g_font_inter_bold_13);
static const Font s_font_title(&font_data::g_font_inter_title_18);
static const Font s_font_caption(&font_data::g_font_inter_caption_11);

const Font& Font::Default() {
    return s_font_regular;
}

const Font& Font::Regular() {
    return s_font_regular;
}

const Font& Font::Bold() {
    return s_font_bold;
}

const Font& Font::Title() {
    return s_font_title;
}

const Font& Font::Caption() {
    return s_font_caption;
}

} // namespace bos
