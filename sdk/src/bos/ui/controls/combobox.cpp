#include "bos/controls/combobox.hpp"
#include <string.h>

namespace bos {

ComboBox::ComboBox() {
    set_focusable(true);
    set_size(180, Theme::ControlHeight());
}

bool ComboBox::add_item(const char* item) {
    if (m_item_count >= MAX_ITEMS || !item) return false;

    strncpy(m_items[m_item_count], item, sizeof(m_items[0]) - 1);
    m_items[m_item_count][sizeof(m_items[0]) - 1] = '\0';

    if (m_selected_index < 0) {
        m_selected_index = 0;
    }
    m_item_count++;
    invalidate();
    return true;
}

void ComboBox::clear_items() {
    m_item_count = 0;
    m_selected_index = -1;
    m_dropped_down = false;
    invalidate();
}

void ComboBox::set_selected_index(int32_t index) {
    if (index >= 0 && index < (int32_t)m_item_count && m_selected_index != index) {
        m_selected_index = index;
        invalidate();
        if (m_on_selection) {
            m_on_selection(this, m_selected_index, selected_item(), m_user_data);
        }
    }
}

void ComboBox::set_dropped_down(bool dropped) {
    if (m_dropped_down != dropped) {
        m_dropped_down = dropped;
        invalidate();
    }
}

void ComboBox::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();
    Rect header_rect(abs_r.x, abs_r.y, abs_r.width, Theme::ControlHeight());

    // 1. Draw Closed Header Box
    Color bg = Theme::SurfaceSubtle();
    Color bdr = !enabled() ? Theme::Border() : (is_focused() ? Theme::Accent() : (is_hovered() ? Theme::BorderHighlight() : Theme::Border()));

    surface.fill_rounded_rect(header_rect, Theme::RadiusControl(), bg);
    surface.draw_rounded_rect(header_rect, Theme::RadiusControl(), bdr, is_focused() ? 2 : 1);

    // Selected text
    const char* txt = selected_item();
    if (txt[0] != '\0') {
        int32_t text_y = header_rect.y + ((int32_t)header_rect.height - 16) / 2;
        surface.draw_string(header_rect.x + 10, text_y, txt, enabled() ? Theme::TextPrimary() : Theme::TextMuted());
    }

    // Down Arrow Chevron: 'v'
    int32_t arrow_x = header_rect.right() - 20;
    int32_t arrow_y = header_rect.y + ((int32_t)header_rect.height - 16) / 2;
    surface.draw_string(arrow_x, arrow_y, m_dropped_down ? "^" : "v", Theme::TextSecondary());

    // 2. If Dropped Down: Draw Dropdown List Overlay
    if (m_dropped_down && m_item_count > 0) {
        uint32_t list_h = (uint32_t)(m_item_count * 28 + 6);
        Rect list_rect(abs_r.x, header_rect.bottom() + 2, abs_r.width, list_h);

        surface.fill_rounded_rect(list_rect, Theme::RadiusControl(), Theme::SurfaceCard());
        surface.draw_rounded_rect(list_rect, Theme::RadiusControl(), Theme::BorderHighlight(), 1);

        int32_t cur_y = list_rect.y + 3;
        for (size_t i = 0; i < m_item_count; ++i) {
            Rect row(list_rect.x + 3, cur_y, list_rect.width - 6, 26);
            bool is_sel = ((int32_t)i == m_selected_index);
            bool is_hov = ((int32_t)i == m_hover_index);

            if (is_sel) {
                surface.fill_rounded_rect(row, Theme::RadiusSmall(), Theme::Accent());
            } else if (is_hov) {
                surface.fill_rounded_rect(row, Theme::RadiusSmall(), Color(0xFF2A374D));
            }

            int32_t ty = row.y + ((int32_t)row.height - 16) / 2;
            surface.draw_string(row.x + 8, ty, m_items[i], is_sel ? Color::White() : Theme::TextPrimary());

            cur_y += 28;
        }
    }

    Widget::paint(surface);
}

bool ComboBox::on_event(const Event& event) {
    if (!enabled()) return false;

    Rect abs_r = absolute_bounds();
    Rect header_rect(abs_r.x, abs_r.y, abs_r.width, Theme::ControlHeight());

    if (event.type == EventType::MouseMove && m_dropped_down) {
        int32_t rel_y = event.mouse_pos.y - (header_rect.bottom() + 5);
        int32_t new_hover = -1;
        if (rel_y >= 0 && event.mouse_pos.x >= abs_r.x && event.mouse_pos.x <= abs_r.right()) {
            int32_t idx = rel_y / 28;
            if (idx >= 0 && idx < (int32_t)m_item_count) {
                new_hover = idx;
            }
        }
        if (new_hover != m_hover_index) {
            m_hover_index = new_hover;
            invalidate();
        }
        return true;
    }

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        if (m_dropped_down) {
            if (m_hover_index >= 0 && m_hover_index < (int32_t)m_item_count) {
                set_selected_index(m_hover_index);
            }
            set_dropped_down(false);
            return true;
        } else {
            set_dropped_down(true);
            return true;
        }
    }

    if (event.type == EventType::KeyDown && m_dropped_down) {
        if (event.key_code == 27) { // Escape
            set_dropped_down(false);
            return true;
        }
    }

    return Widget::on_event(event);
}

Size ComboBox::measure_preferred_size() const {
    return Size(180, Theme::ControlHeight());
}

} // namespace bos
