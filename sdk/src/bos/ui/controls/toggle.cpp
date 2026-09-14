#include "bos/controls/toggle.hpp"

namespace bos {

Toggle::Toggle() {
    set_focusable(true);
    set_size(44, 22);
}

Toggle::Toggle(bool is_on) : Toggle() {
    m_is_on = is_on;
}

void Toggle::set_on(bool is_on) {
    if (m_is_on != is_on) {
        m_is_on = is_on;
        invalidate();
        if (m_on_toggle) {
            m_on_toggle(this, m_is_on, m_user_data);
        }
    }
}

void Toggle::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // Pill container (44x22, radius 11)
    uint32_t radius = abs_r.height / 2;
    Color bg = m_is_on ? Theme::Accent() : Theme::SurfaceSubtle();
    Color bdr = !enabled() ? Theme::Border() : (is_focused() ? Theme::FocusRing() : (is_hovered() ? Theme::AccentHover() : (m_is_on ? Theme::Accent() : Theme::Border())));
    uint32_t bdr_thick = is_focused() ? 2 : 1;

    surface.fill_rounded_rect(abs_r, radius, bg);
    surface.draw_rounded_rect(abs_r, radius, bdr, bdr_thick);

    // Circular thumb (16x16, radius 8)
    int32_t thumb_size = (int32_t)abs_r.height - 6;
    int32_t thumb_y = abs_r.y + 3;
    int32_t thumb_x = m_is_on ? (abs_r.right() - thumb_size - 3) : (abs_r.x + 3);

    Rect thumb_rect(thumb_x, thumb_y, thumb_size, thumb_size);
    surface.fill_rounded_rect(thumb_rect, thumb_size / 2, Color::White());

    Widget::paint(surface);
}

bool Toggle::on_event(const Event& event) {
    if (!enabled()) return false;

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        set_on(!m_is_on);
        return true;
    }

    if (event.type == EventType::KeyDown && event.key_code == 32) { // Space
        set_on(!m_is_on);
        return true;
    }

    return Widget::on_event(event);
}

Size Toggle::measure_preferred_size() const {
    return Size(44, 22);
}

} // namespace bos
