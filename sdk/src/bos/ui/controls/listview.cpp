#include "bos/controls/listview.hpp"
#include <string.h>

namespace bos {

ListView::ListView() {
    set_focusable(true);
    set_size(240, 160);
}

bool ListView::add_item(const char* text, const Icon& icon, const char* detail) {
    if (m_item_count >= MAX_ITEMS || !text) return false;

    ListViewItem& it = m_items[m_item_count++];
    strncpy(it.text, text, sizeof(it.text) - 1);
    it.text[sizeof(it.text) - 1] = '\0';

    if (detail) {
        strncpy(it.detail, detail, sizeof(it.detail) - 1);
        it.detail[sizeof(it.detail) - 1] = '\0';
    } else {
        it.detail[0] = '\0';
    }

    it.icon = icon;
    invalidate();
    return true;
}

void ListView::clear_items() {
    m_item_count = 0;
    m_selected_index = -1;
    m_hover_index = -1;
    invalidate();
}

void ListView::set_selected_index(int32_t index) {
    if (index >= -1 && index < (int32_t)m_item_count && m_selected_index != index) {
        m_selected_index = index;
        invalidate();
        if (m_selected_index >= 0 && m_on_item_click) {
            m_on_item_click(this, m_selected_index, m_user_data);
        }
    }
}

void ListView::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // Container Background & Border
    surface.fill_rounded_rect(abs_r, Theme::RadiusControl(), Theme::SurfaceSubtle());
    surface.draw_rounded_rect(abs_r, Theme::RadiusControl(), is_focused() ? Theme::Accent() : Theme::Border(), 1);

    int32_t cur_y = abs_r.y + 4;
    int32_t item_w = (int32_t)abs_r.width - 8;

    for (size_t i = 0; i < m_item_count; ++i) {
        if (cur_y + (int32_t)m_item_height > abs_r.bottom() - 4) break;

        Rect row_rect(abs_r.x + 4, cur_y, (uint32_t)item_w, m_item_height);
        bool is_sel = ((int32_t)i == m_selected_index);
        bool is_hov = ((int32_t)i == m_hover_index);

        if (is_sel) {
            surface.fill_rounded_rect(row_rect, Theme::RadiusSmall(), Theme::Accent());
        } else if (is_hov) {
            surface.fill_rounded_rect(row_rect, Theme::RadiusSmall(), Color(0xFF2A374D));
        }

        const ListViewItem& it = m_items[i];
        int32_t content_x = row_rect.x + 8;

        // Draw Icon if available
        if (it.icon.is_valid()) {
            int32_t icon_y = row_rect.y + ((int32_t)m_item_height - (int32_t)it.icon.size().height) / 2;
            it.icon.draw(surface, Point(content_x, icon_y));
            content_x += it.icon.size().width + 8;
        }

        // Draw Item Text
        int32_t text_y = row_rect.y + ((int32_t)m_item_height - 16) / 2;
        Color text_color = is_sel ? Color::White() : Theme::TextPrimary();
        surface.draw_string(content_x, text_y, it.text, text_color);

        // Draw Detail / Subtitle on Right
        if (it.detail[0] != '\0') {
            Size det_sz = surface.measure_string(it.detail);
            int32_t det_x = row_rect.right() - (int32_t)det_sz.width - 8;
            if (det_x > content_x + 50) {
                Color det_col = is_sel ? Color(0xFFE2E8F0) : Theme::TextSecondary();
                surface.draw_string(det_x, text_y, it.detail, det_col);
            }
        }

        cur_y += m_item_height + 2;
    }

    Widget::paint(surface);
}

bool ListView::on_event(const Event& event) {
    if (!enabled()) return false;

    Rect abs_r = absolute_bounds();

    if (event.type == EventType::MouseMove) {
        int32_t rel_y = event.mouse_pos.y - (abs_r.y + 4);
        int32_t new_hover = -1;
        if (rel_y >= 0 && event.mouse_pos.x >= abs_r.x && event.mouse_pos.x <= abs_r.right()) {
            int32_t idx = rel_y / ((int32_t)m_item_height + 2);
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

    if (event.type == EventType::MouseLeave) {
        if (m_hover_index != -1) {
            m_hover_index = -1;
            invalidate();
        }
    }

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        if (m_hover_index >= 0 && m_hover_index < (int32_t)m_item_count) {
            set_selected_index(m_hover_index);
            return true;
        }
    }

    if (event.type == EventType::KeyDown) {
        if (event.key_code == 38) { // Up arrow
            if (m_selected_index > 0) {
                set_selected_index(m_selected_index - 1);
                return true;
            }
        } else if (event.key_code == 40) { // Down arrow
            if (m_selected_index + 1 < (int32_t)m_item_count) {
                set_selected_index(m_selected_index + 1);
                return true;
            }
        }
    }

    return Widget::on_event(event);
}

Size ListView::measure_preferred_size() const {
    return Size(240, 40 + m_item_count * (m_item_height + 2));
}

} // namespace bos
