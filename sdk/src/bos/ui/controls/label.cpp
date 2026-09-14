#include "bos/controls/label.hpp"
#include <string.h>

namespace bos {

Label::Label() {
    set_size(100, 20);
}

Label::Label(const char* text, Color color) : Label() {
    set_text(text);
    m_color = color;
}

void Label::set_text(const char* text) {
    if (text) {
        strncpy(m_text, text, sizeof(m_text) - 1);
        m_text[sizeof(m_text) - 1] = '\0';
    } else {
        m_text[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void Label::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();
    Color draw_color = m_color;

    if (!enabled()) {
        draw_color = Theme::TextMuted();
    } else if (m_is_link && is_hovered()) {
        draw_color = Theme::AccentHover();
    }

    Size sz = surface.measure_string(m_text);
    int32_t x = abs_r.x;

    if (m_alignment == Alignment::Center) {
        x += ((int32_t)abs_r.width - (int32_t)sz.width) / 2;
    } else if (m_alignment == Alignment::End) {
        x += ((int32_t)abs_r.width - (int32_t)sz.width);
    }

    int32_t y = abs_r.y + ((int32_t)abs_r.height - (int32_t)sz.height) / 2;
    surface.draw_string(x, y, m_text, draw_color);

    if (m_is_link) {
        int32_t underline_y = y + sz.height;
        surface.draw_line(x, underline_y, x + sz.width, underline_y, draw_color);
    }

    Widget::paint(surface);
}

bool Label::on_event(const Event& event) {
    if (m_is_link && enabled()) {
        if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
            if (m_on_click) {
                m_on_click(this, m_user_data);
            }
            return true;
        }
    }
    return Widget::on_event(event);
}

Size Label::measure_preferred_size() const {
    const Font& f = Font::Regular();
    TextMetrics tm = f.measure(m_text);
    uint32_t h = (tm.height > 16) ? tm.height : 18;
    return Size(tm.width, h);
}

} // namespace bos
