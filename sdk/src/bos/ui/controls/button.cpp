#include "bos/controls/button.hpp"
#include <string.h>

namespace bos {

Button::Button() {
    set_focusable(true);
    set_size(120, Theme::ControlHeight());
}

Button::Button(const char* text) : Button() {
    set_text(text);
}

Button::Button(const char* text, const Icon& icon) : Button() {
    set_text(text);
    set_icon(icon);
}

Button::Button(const char* text, ButtonVariant variant) : Button() {
    set_text(text);
    set_variant(variant);
}

void Button::set_variant(ButtonVariant v) {
    m_variant = v;
    if (v == ButtonVariant::Primary) {
        set_style(Theme::make_primary_button_style());
    } else if (v == ButtonVariant::Ghost) {
        set_style(Theme::make_ghost_button_style());
    } else {
        set_style(Theme::make_secondary_button_style());
    }
    invalidate();
}

void Button::set_text(const char* text) {
    if (text) {
        strncpy(m_text, text, sizeof(m_text) - 1);
        m_text[sizeof(m_text) - 1] = '\0';
    } else {
        m_text[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void Button::set_icon(const Icon& icon) {
    m_icon = icon;
    invalidate();
}

void Button::set_selected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        invalidate();
    }
}

VisualState Button::current_visual_state() const {
    if (!enabled()) return VisualState::Disabled;
    if (m_selected) return VisualState::Selected;
    if (m_pressed)  return VisualState::Pressed;
    if (is_hovered()) return VisualState::Hover;
    if (is_focused()) return VisualState::Focused;
    return VisualState::Normal;
}

void Button::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();
    const StateStyle& st = m_style.resolve(current_visual_state());

    // 1. Draw Background
    if (st.has_image()) {
        surface.draw_image_9slice(st.image_asset, abs_r, st.slice_borders);
    } else {
        surface.fill_rounded_rect(abs_r, st.corner_radius, st.background);
        if (st.border_thickness > 0) {
            surface.draw_rounded_rect(abs_r, st.corner_radius, st.border, st.border_thickness);
        }
    }

    // 2. High-Contrast Accessible Focus Ring (Offset by 2px outside)
    if (is_focused()) {
        Rect focus_r(abs_r.x - 2, abs_r.y - 2, abs_r.width + 4, abs_r.height + 4);
        surface.draw_rounded_rect(focus_r, st.corner_radius + 2, Theme::FocusRing(), 2);
    }

    // 3. Measure & Layout Icon + Inter Font Text
    const Font& btn_font = (m_variant == ButtonVariant::Primary) ? Font::Bold() : Font::Regular();
    TextMetrics tm = surface.measure_string(m_text, btn_font);
    uint32_t icon_w = m_icon.is_valid() ? m_icon.size().width : 0;
    uint32_t icon_h = m_icon.is_valid() ? m_icon.size().height : 0;
    uint32_t gap = (m_icon.is_valid() && m_text[0] != '\0') ? 8 : 0;

    uint32_t total_content_w = icon_w + gap + tm.width;
    int32_t start_x = abs_r.x + ((int32_t)abs_r.width - (int32_t)total_content_w) / 2;
    if (start_x < abs_r.x + 6) start_x = abs_r.x + 6;

    // Draw Icon
    if (m_icon.is_valid()) {
        int32_t icon_y = abs_r.y + ((int32_t)abs_r.height - (int32_t)icon_h) / 2;
        m_icon.draw(surface, Point(start_x, icon_y));
        start_x += icon_w + gap;
    }

    // Draw Text with Inter Font
    if (m_text[0] != '\0') {
        int32_t text_y = abs_r.y + ((int32_t)abs_r.height - (int32_t)tm.height) / 2;
        surface.draw_string(start_x, text_y, m_text, btn_font, st.foreground);
    }

    Widget::paint(surface);
}

bool Button::on_event(const Event& event) {
    if (!enabled()) return false;

    if (event.type == EventType::MouseDown && event.mouse_button == MouseButton::Left) {
        m_pressed = true;
        set_mouse_capture();
        invalidate();
        return true;
    }

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        if (m_pressed) {
            m_pressed = false;
            release_mouse_capture();
            invalidate();

            Rect local_b(0, 0, m_bounds.width, m_bounds.height);
            if (local_b.contains(event.mouse_pos)) {
                if (m_on_click) {
                    m_on_click(this, m_user_data);
                }
            }
            return true;
        }
    }

    if (event.type == EventType::MouseLeave) {
        if (m_pressed) {
            m_pressed = false;
            invalidate();
        }
    }

    if (event.type == EventType::KeyDown && (event.key_code == 13 || event.key_code == 32)) { // Enter or Space
        if (m_on_click) {
            m_on_click(this, m_user_data);
        }
        return true;
    }

    return Widget::on_event(event);
}

Size Button::measure_preferred_size() const {
    const Font& btn_font = (m_variant == ButtonVariant::Primary) ? Font::Bold() : Font::Regular();
    TextMetrics tm = btn_font.measure(m_text);
    uint32_t icon_w = m_icon.is_valid() ? m_icon.size().width : 0;
    uint32_t gap = (m_icon.is_valid() && m_text[0] != '\0') ? 8 : 0;
    uint32_t pad_h = 24; // 12px left + 12px right padding
    uint32_t w = icon_w + gap + tm.width + pad_h;
    if (w < 48) w = 48; // minimum button width for "OK"
    return Size(w, Theme::ButtonHeight());
}

} // namespace bos
