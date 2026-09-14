#include "bos/controls/checkbox.hpp"
#include <string.h>

namespace bos {

CheckBox::CheckBox() {
    set_focusable(true);
    set_size(120, 24);
}

CheckBox::CheckBox(const char* text, bool checked) : CheckBox() {
    set_text(text);
    m_checked = checked;
}

void CheckBox::set_text(const char* text) {
    if (text) {
        strncpy(m_text, text, sizeof(m_text) - 1);
        m_text[sizeof(m_text) - 1] = '\0';
    } else {
        m_text[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void CheckBox::set_checked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        invalidate();
        if (m_on_toggle) {
            m_on_toggle(this, m_checked, m_user_data);
        }
    }
}

void CheckBox::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Box dimensions (18x18, vertically centered)
    int32_t box_size = 18;
    int32_t box_y = abs_r.y + ((int32_t)abs_r.height - box_size) / 2;
    Rect box_rect(abs_r.x, box_y, box_size, box_size);

    Color bg = m_checked ? Theme::Accent() : Theme::SurfaceSubtle();
    Color bdr = !enabled() ? Theme::Border() : (is_focused() ? Theme::FocusRing() : (is_hovered() ? Theme::AccentHover() : Theme::Border()));
    uint32_t bdr_thick = is_focused() ? 2 : 1;

    surface.fill_rounded_rect(box_rect, Theme::RadiusSmall(), bg);
    surface.draw_rounded_rect(box_rect, Theme::RadiusSmall(), bdr, bdr_thick);

    // 2. Draw Checkmark if checked
    if (m_checked) {
        // Draw anti-aliased or sharp checkmark lines: (x+4, y+9) -> (x+7, y+13) -> (x+13, y+5)
        int32_t cx = box_rect.x;
        int32_t cy = box_rect.y;
        surface.draw_line(cx + 4, cy + 9,  cx + 7,  cy + 13, Color::White());
        surface.draw_line(cx + 5, cy + 9,  cx + 8,  cy + 13, Color::White());
        surface.draw_line(cx + 7, cy + 13, cx + 13, cy + 5,  Color::White());
        surface.draw_line(cx + 8, cy + 13, cx + 14, cy + 5,  Color::White());
    }

    // 3. Draw Label Text
    if (m_text[0] != '\0') {
        int32_t text_x = box_rect.right() + 8;
        int32_t text_y = abs_r.y + ((int32_t)abs_r.height - 16) / 2;
        Color text_color = enabled() ? Theme::TextPrimary() : Theme::TextMuted();
        surface.draw_string(text_x, text_y, m_text, text_color);
    }

    Widget::paint(surface);
}

bool CheckBox::on_event(const Event& event) {
    if (!enabled()) return false;

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        set_checked(!m_checked);
        return true;
    }

    if (event.type == EventType::KeyDown && event.key_code == 32) { // Space
        set_checked(!m_checked);
        return true;
    }

    return Widget::on_event(event);
}

Size CheckBox::measure_preferred_size() const {
    const Font& f = Font::Regular();
    TextMetrics tm = f.measure(m_text);
    return Size(18 + 8 + tm.width, 24);
}

} // namespace bos
