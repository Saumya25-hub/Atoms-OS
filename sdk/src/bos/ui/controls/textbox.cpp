#include "bos/controls/textbox.hpp"
#include <string.h>

namespace bos {

TextBox::TextBox() {
    set_focusable(true);
    set_size(180, Theme::ControlHeight());
}

TextBox::TextBox(const char* placeholder) : TextBox() {
    set_placeholder(placeholder);
}

void TextBox::set_text(const char* text) {
    if (text) {
        strncpy(m_text, text, sizeof(m_text) - 1);
        m_text[sizeof(m_text) - 1] = '\0';
        m_cursor_pos = strlen(m_text);
    } else {
        m_text[0] = '\0';
        m_cursor_pos = 0;
    }
    invalidate_layout();
    invalidate();
}

void TextBox::set_placeholder(const char* placeholder) {
    if (placeholder) {
        strncpy(m_placeholder, placeholder, sizeof(m_placeholder) - 1);
        m_placeholder[sizeof(m_placeholder) - 1] = '\0';
    } else {
        m_placeholder[0] = '\0';
    }
    invalidate_layout();
    invalidate();
}

void TextBox::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // Background & Border
    Color bg = enabled() ? Theme::SurfaceSubtle() : Color(0xFF111827);
    Color bdr = !enabled() ? Theme::Border() : (is_focused() ? Theme::FocusRing() : (is_hovered() ? Theme::BorderHighlight() : Theme::Border()));
    uint32_t thick = is_focused() ? 2 : 1;

    surface.fill_rounded_rect(abs_r, Theme::RadiusControl(), bg);
    surface.draw_rounded_rect(abs_r, Theme::RadiusControl(), bdr, thick);

    const Font& tb_font = Font::Regular();
    int32_t content_x = abs_r.x + 10;
    int32_t content_y = abs_r.y + ((int32_t)abs_r.height - (int32_t)tb_font.line_height()) / 2;

    if (m_text[0] != '\0') {
        Color fg = enabled() ? Theme::TextPrimary() : Theme::TextMuted();
        surface.draw_string(content_x, content_y, m_text, tb_font, fg);
    } else if (m_placeholder[0] != '\0') {
        surface.draw_string(content_x, content_y, m_placeholder, tb_font, Theme::TextMuted());
    }

    // Blinking / Active cursor (measured proportionally with Inter font)
    if (is_focused() && enabled()) {
        char sub[128];
        size_t cp_len = (m_cursor_pos < sizeof(sub) - 1) ? m_cursor_pos : (sizeof(sub) - 1);
        strncpy(sub, m_text, cp_len);
        sub[cp_len] = '\0';
        int32_t cursor_offset = tb_font.measure(sub).width;
        int32_t cursor_x = content_x + cursor_offset;
        if (cursor_x < abs_r.right() - 6) {
            surface.draw_line(cursor_x, content_y + 1, cursor_x, content_y + tb_font.line_height() - 2, Theme::Accent());
        }
    }

    Widget::paint(surface);
}

bool TextBox::on_event(const Event& event) {
    if (!enabled()) return false;

    if (event.type == EventType::MouseDown && event.mouse_button == MouseButton::Left) {
        // Focus is acquired on click
        return true;
    }

    if (!is_focused() || m_read_only) return false;

    if (event.type == EventType::KeyDown) {
        size_t len = strlen(m_text);

        if (event.key_code == 8) { // Backspace
            if (m_cursor_pos > 0) {
                for (size_t i = m_cursor_pos - 1; i < len; ++i) {
                    m_text[i] = m_text[i + 1];
                }
                m_cursor_pos--;
                invalidate();
                if (m_on_text_changed) m_on_text_changed(this, m_user_data);
                return true;
            }
        } else if (event.key_code == 37) { // Left arrow
            if (m_cursor_pos > 0) {
                m_cursor_pos--;
                invalidate();
                return true;
            }
        } else if (event.key_code == 39) { // Right arrow
            if (m_cursor_pos < len) {
                m_cursor_pos++;
                invalidate();
                return true;
            }
        } else if (event.ascii_char >= 32 && event.ascii_char <= 126) {
            if (len + 1 < sizeof(m_text)) {
                for (size_t i = len + 1; i > m_cursor_pos; --i) {
                    m_text[i] = m_text[i - 1];
                }
                m_text[m_cursor_pos] = (char)event.ascii_char;
                m_cursor_pos++;
                invalidate();
                if (m_on_text_changed) m_on_text_changed(this, m_user_data);
                return true;
            }
        }
    }

    return Widget::on_event(event);
}

Size TextBox::measure_preferred_size() const {
    const Font& tb_font = Font::Regular();
    const char* str = (m_text[0] != '\0') ? m_text : m_placeholder;
    TextMetrics tm = tb_font.measure(str);
    uint32_t w = tm.width + 24;
    if (w < 140) w = 140;
    return Size(w, Theme::InputHeight());
}

} // namespace bos
