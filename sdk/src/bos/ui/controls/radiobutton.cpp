#include "bos/controls/radiobutton.hpp"
#include <string.h>

namespace bos {

RadioButton::RadioButton() {
    set_focusable(true);
    set_size(120, 24);
}

RadioButton::RadioButton(const char* text, uint32_t group_id, bool selected) : RadioButton() {
    set_text(text);
    m_group_id = group_id;
    m_selected = selected;
}

void RadioButton::set_text(const char* text) {
    if (text) {
        strncpy(m_text, text, sizeof(m_text) - 1);
        m_text[sizeof(m_text) - 1] = '\0';
    } else {
        m_text[0] = '\0';
    }
    invalidate();
}

void RadioButton::set_selected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        invalidate();

        if (m_selected) {
            // Unselect sibling RadioButtons with the same group_id
            if (parent()) {
                for (size_t i = 0; i < parent()->child_count(); ++i) {
                    Widget* sibling = parent()->child_at(i);
                    if (sibling && sibling != this) {
                        RadioButton* rb = static_cast<RadioButton*>(sibling);
                        if (rb && rb->group_id() == m_group_id && rb->is_selected()) {
                            rb->m_selected = false;
                            rb->invalidate();
                        }
                    }
                }
            }

            if (m_on_select) {
                m_on_select(this, m_group_id, m_user_data);
            }
        }
    }
}

void RadioButton::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // Circle bounds (18x18 diameter, vertically centered)
    int32_t circle_size = 18;
    int32_t circle_y = abs_r.y + ((int32_t)abs_r.height - circle_size) / 2;
    Rect circle_rect(abs_r.x, circle_y, circle_size, circle_size);

    Color bg = Theme::SurfaceSubtle();
    Color bdr = !enabled() ? Theme::Border() : (is_hovered() ? Theme::AccentHover() : Theme::Border());

    surface.fill_rounded_rect(circle_rect, circle_size / 2, bg);
    surface.draw_rounded_rect(circle_rect, circle_size / 2, bdr, 1);

    // Inner filled bullet if selected
    if (m_selected) {
        int32_t bullet_size = 8;
        int32_t bullet_x = circle_rect.x + (circle_size - bullet_size) / 2;
        int32_t bullet_y = circle_rect.y + (circle_size - bullet_size) / 2;
        Rect bullet_rect(bullet_x, bullet_y, bullet_size, bullet_size);
        Color bullet_color = enabled() ? Theme::Accent() : Theme::TextMuted();
        surface.fill_rounded_rect(bullet_rect, bullet_size / 2, bullet_color);
    }

    // Label Text
    if (m_text[0] != '\0') {
        int32_t text_x = circle_rect.right() + 8;
        int32_t text_y = abs_r.y + ((int32_t)abs_r.height - 16) / 2;
        Color text_color = enabled() ? Theme::TextPrimary() : Theme::TextMuted();
        surface.draw_string(text_x, text_y, m_text, text_color);
    }

    Widget::paint(surface);
}

bool RadioButton::on_event(const Event& event) {
    if (!enabled()) return false;

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        set_selected(true);
        return true;
    }

    if (event.type == EventType::KeyDown && event.key_code == 32) { // Space
        set_selected(true);
        return true;
    }

    return Widget::on_event(event);
}

Size RadioButton::measure_preferred_size() const {
    uint32_t len = (uint32_t)strlen(m_text);
    return Size(26 + len * 8, 24);
}

} // namespace bos
