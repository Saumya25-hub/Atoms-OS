#include "bos/controls/card.hpp"
#include <string.h>

namespace bos {

Card::Card() {
    set_size(240, 140);
}

Card::Card(const char* title, const Icon& icon) : Card() {
    set_title(title);
    set_icon(icon);
}

void Card::set_title(const char* title) {
    if (title) {
        strncpy(m_title, title, sizeof(m_title) - 1);
        m_title[sizeof(m_title) - 1] = '\0';
    } else {
        m_title[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void Card::set_subtitle(const char* subtitle) {
    if (subtitle) {
        strncpy(m_subtitle, subtitle, sizeof(m_subtitle) - 1);
        m_subtitle[sizeof(m_subtitle) - 1] = '\0';
    } else {
        m_subtitle[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void Card::set_icon(const Icon& icon) {
    m_icon = icon;
    invalidate();
}

void Card::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Draw Background (Flat rounded or 9-slice)
    if (m_bg_image.is_valid()) {
        surface.draw_image_9slice(m_bg_image, abs_r, m_slice_borders);
    } else {
        surface.fill_rounded_rect(abs_r, Theme::RadiusCard(), m_bg_color);
        surface.draw_rounded_rect(abs_r, Theme::RadiusCard(), m_border_color, 1);
    }

    // 2. Draw Header Area if title/icon present
    int32_t content_x = abs_r.x + 12;
    int32_t cur_y = abs_r.y + 12;

    if (m_icon.is_valid()) {
        m_icon.draw(surface, Point(content_x, cur_y));
        content_x += m_icon.size().width + 10;
    }

    if (m_title[0] != '\0') {
        surface.draw_string(content_x, cur_y, m_title, Font::Bold(), Theme::TextPrimary());
        if (m_subtitle[0] != '\0') {
            surface.draw_string(content_x, cur_y + 20, m_subtitle, Font::Caption(), Theme::TextSecondary());
        }
    }

    Widget::paint(surface);
}

Size Card::measure_preferred_size() const {
    uint32_t header_h = 0;
    uint32_t header_w = 0;
    if (m_title[0] != '\0') {
        TextMetrics tm = Font::Bold().measure(m_title);
        header_w = tm.width + (m_icon.is_valid() ? m_icon.size().width + 10 : 0) + 24;
        header_h = 28;
        if (m_subtitle[0] != '\0') {
            TextMetrics stm = Font::Caption().measure(m_subtitle);
            if ((uint32_t)stm.width + 24 > header_w) header_w = stm.width + 24;
            header_h = 48;
        }
    }

    uint32_t children_w = 0;
    uint32_t children_h = 0;
    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] && m_children[i]->visible()) {
            Size cs = m_children[i]->measure_preferred_size();
            Insets m = m_children[i]->margin();
            uint32_t cw = cs.width + m.horizontal();
            uint32_t ch = cs.height + m.vertical();
            if (cw > children_w) children_w = cw;
            children_h += ch + 8;
        }
    }

    uint32_t total_w = (children_w > header_w) ? children_w : header_w;
    total_w += m_padding.horizontal();
    if (total_w < 180) total_w = 180;

    uint32_t total_h = header_h + children_h + m_padding.vertical();
    if (total_h < 80) total_h = 80;

    return Size(total_w, total_h);
}

} // namespace bos
