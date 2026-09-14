#include "bos/controls/smart_panel.hpp"
#include <string.h>

namespace bos {

SmartPanel::SmartPanel(Orientation orientation, SizingMode sizing)
    : m_orientation(orientation),
      m_sizing_mode(sizing),
      m_alignment(Alignment::Start),
      m_spacing(Theme::PanelSpacing()) {
    // Default padding from centralized design token
    uint32_t pad = Theme::PanelPadding();
    set_padding(Insets(pad, pad, pad, pad));
}

SmartPanel& SmartPanel::add(Widget* child) {
    if (child) {
        add_child(child);
        invalidate_layout();
        perform_layout();
    }
    return *this;
}

void SmartPanel::set_orientation(Orientation o) {
    if (m_orientation != o) {
        m_orientation = o;
        invalidate_layout();
        perform_layout();
        invalidate();
    }
}

void SmartPanel::set_sizing_mode(SizingMode mode) {
    if (m_sizing_mode != mode) {
        m_sizing_mode = mode;
        invalidate_layout();
        perform_layout();
        invalidate();
    }
}

void SmartPanel::set_alignment(Alignment align) {
    if (m_alignment != align) {
        m_alignment = align;
        invalidate_layout();
        perform_layout();
        invalidate();
    }
}

void SmartPanel::set_spacing(int32_t spacing) {
    if (m_spacing != spacing) {
        m_spacing = spacing;
        invalidate_layout();
        perform_layout();
        invalidate();
    }
}

void SmartPanel::set_card_style(bool enable, Color bg, Color border, uint32_t radius) {
    m_card_style = enable;
    m_bg_color = bg;
    m_border_color = border;
    m_corner_radius = radius;
    invalidate();
}

void SmartPanel::set_title(const char* title, const char* subtitle) {
    if (title) {
        strncpy(m_title, title, sizeof(m_title) - 1);
        m_title[sizeof(m_title) - 1] = '\0';
    } else {
        m_title[0] = '\0';
    }

    if (subtitle) {
        strncpy(m_subtitle, subtitle, sizeof(m_subtitle) - 1);
        m_subtitle[sizeof(m_subtitle) - 1] = '\0';
    } else {
        m_subtitle[0] = '\0';
    }

    invalidate_layout();
    perform_layout();
    invalidate();
}

Size SmartPanel::measure_preferred_size() const {
    uint32_t header_w = 0;
    uint32_t header_h = 0;

    if (m_title[0] != '\0') {
        TextMetrics tm = Font::Bold().measure(m_title);
        header_w = (uint32_t)tm.width;
        header_h = 24;

        if (m_subtitle[0] != '\0') {
            TextMetrics stm = Font::Caption().measure(m_subtitle);
            if ((uint32_t)stm.width > header_w) {
                header_w = (uint32_t)stm.width;
            }
            header_h = 42;
        }
        header_h += 8; // gap between header and content
    }

    uint32_t total_w = 0;
    uint32_t total_h = 0;
    size_t visible_children = 0;

    if (m_orientation == Orientation::Vertical) {
        uint32_t max_cw = 0;
        uint32_t sum_ch = 0;

        for (size_t i = 0; i < m_child_count; ++i) {
            Widget* child = m_children[i];
            if (!child || !child->visible()) continue;

            visible_children++;
            Size cs = child->measure_preferred_size();
            Insets m = child->margin();
            uint32_t cw = cs.width + m.horizontal();
            uint32_t ch = cs.height + m.vertical();

            if (cw > max_cw) max_cw = cw;
            sum_ch += ch;
        }

        if (visible_children > 1) {
            sum_ch += (uint32_t)(visible_children - 1) * m_spacing;
        }

        total_w = (max_cw > header_w) ? max_cw : header_w;
        total_h = header_h + sum_ch;
    } else {
        // Horizontal orientation
        uint32_t sum_cw = 0;
        uint32_t max_ch = 0;

        for (size_t i = 0; i < m_child_count; ++i) {
            Widget* child = m_children[i];
            if (!child || !child->visible()) continue;

            visible_children++;
            Size cs = child->measure_preferred_size();
            Insets m = child->margin();
            uint32_t cw = cs.width + m.horizontal();
            uint32_t ch = cs.height + m.vertical();

            sum_cw += cw;
            if (ch > max_ch) max_ch = ch;
        }

        if (visible_children > 1) {
            sum_cw += (uint32_t)(visible_children - 1) * m_spacing;
        }

        total_w = (sum_cw > header_w) ? sum_cw : header_w;
        total_h = (max_ch > header_h) ? max_ch : header_h;
    }

    total_w += m_padding.horizontal();
    total_h += m_padding.vertical();

    return Size(total_w, total_h);
}

void SmartPanel::perform_layout() {
    if (m_in_layout) return;
    m_in_layout = true;

    Size pref = measure_preferred_size();

    // Auto-framing: adjust own bounds based on sizing mode
    uint32_t new_w = m_bounds.width;
    uint32_t new_h = m_bounds.height;

    if (m_sizing_mode == SizingMode::Auto) {
        new_w = pref.width;
        new_h = pref.height;
    } else if (m_sizing_mode == SizingMode::AutoWidth) {
        new_w = pref.width;
    } else if (m_sizing_mode == SizingMode::AutoHeight) {
        new_h = pref.height;
    }

    if (new_w != m_bounds.width || new_h != m_bounds.height) {
        m_bounds.width = new_w;
        m_bounds.height = new_h;
    }

    // Content area inside padding
    Rect content = content_bounds();

    // Account for header if title is present
    if (m_title[0] != '\0') {
        uint32_t header_h = (m_subtitle[0] != '\0') ? 42 : 24;
        header_h += 8; // header gap
        if (content.height > header_h) {
            content.y += header_h;
            content.height -= header_h;
        }
    }

    if (m_orientation == Orientation::Vertical) {
        int32_t cur_y = content.y;

        for (size_t i = 0; i < m_child_count; ++i) {
            Widget* child = m_children[i];
            if (!child || !child->visible()) continue;

            Insets m = child->margin();
            Size cpref = child->measure_preferred_size();
            uint32_t child_h = (cpref.height > 0) ? cpref.height : child->bounds().height;
            if (child_h == 0) child_h = 30;

            int32_t child_x = content.x + m.left;
            uint32_t child_w = 0;

            if (m_alignment == Alignment::Stretch) {
                child_w = (content.width > (uint32_t)m.horizontal()) ? (content.width - m.horizontal()) : 0;
            } else {
                // Small controls stay small!
                child_w = (cpref.width > 0) ? cpref.width : child->bounds().width;
                if (child_w > content.width - m.horizontal()) {
                    child_w = content.width - m.horizontal();
                }
                if (m_alignment == Alignment::Center) {
                    child_x = content.x + m.left + ((int32_t)(content.width - m.horizontal()) - (int32_t)child_w) / 2;
                } else if (m_alignment == Alignment::End) {
                    child_x = content.right() - m.right - (int32_t)child_w;
                }
            }

            child->set_bounds(Rect(child_x, cur_y + m.top, child_w, child_h));
            cur_y += child_h + m.vertical() + m_spacing;
        }
    } else {
        // Horizontal orientation
        int32_t cur_x = content.x;

        for (size_t i = 0; i < m_child_count; ++i) {
            Widget* child = m_children[i];
            if (!child || !child->visible()) continue;

            Insets m = child->margin();
            Size cpref = child->measure_preferred_size();
            uint32_t child_w = (cpref.width > 0) ? cpref.width : child->bounds().width;
            if (child_w == 0) child_w = 60;

            int32_t child_y = content.y + m.top;
            uint32_t child_h = 0;

            if (m_alignment == Alignment::Stretch) {
                child_h = (content.height > (uint32_t)m.vertical()) ? (content.height - m.vertical()) : 0;
            } else {
                child_h = (cpref.height > 0) ? cpref.height : child->bounds().height;
                if (child_h > content.height - m.vertical()) {
                    child_h = content.height - m.vertical();
                }
                if (m_alignment == Alignment::Center) {
                    child_y = content.y + m.top + ((int32_t)(content.height - m.vertical()) - (int32_t)child_h) / 2;
                } else if (m_alignment == Alignment::End) {
                    child_y = content.bottom() - m.bottom - (int32_t)child_h;
                }
            }

            child->set_bounds(Rect(cur_x + m.left, child_y, child_w, child_h));
            cur_x += child_w + m.horizontal() + m_spacing;
        }
    }

    // Children layout
    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] && m_children[i]->visible()) {
            m_children[i]->perform_layout();
        }
    }

    m_layout_dirty = false;
    m_in_layout = false;
}

void SmartPanel::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Draw Card Surface if enabled
    if (m_card_style) {
        surface.fill_rounded_rect(abs_r, m_corner_radius, m_bg_color);
        surface.draw_rounded_rect(abs_r, m_corner_radius, m_border_color, 1);
    }

    // 2. Draw Title / Subtitle if present
    if (m_title[0] != '\0') {
        int32_t tx = abs_r.x + m_padding.left;
        int32_t ty = abs_r.y + m_padding.top;
        surface.draw_string(tx, ty, m_title, Font::Bold(), Theme::TextPrimary());

        if (m_subtitle[0] != '\0') {
            surface.draw_string(tx, ty + 20, m_subtitle, Font::Caption(), Theme::TextSecondary());
        }
    }

    // 3. Paint children
    Widget::paint(surface);
}

} // namespace bos
