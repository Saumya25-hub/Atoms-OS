#include "bos/controls/sidebar.hpp"
#include <string.h>

namespace bos {

Sidebar::Sidebar() {
    set_size(180, 400);
}

bool Sidebar::add_item(const char* label, const Icon& icon) {
    if (m_item_count >= MAX_ITEMS || !label) return false;

    SidebarItem& it = m_items[m_item_count++];
    strncpy(it.label, label, sizeof(it.label) - 1);
    it.label[sizeof(it.label) - 1] = '\0';
    it.icon = icon;

    invalidate();
    return true;
}

void Sidebar::clear_items() {
    m_item_count = 0;
    m_selected_index = 0;
    m_hover_index = -1;
    invalidate();
}

void Sidebar::set_selected_index(int32_t index) {
    if (index >= 0 && index < (int32_t)m_item_count && m_selected_index != index) {
        m_selected_index = index;
        invalidate();
        if (m_on_selected) {
            m_on_selected(this, m_selected_index, m_items[m_selected_index].label, m_user_data);
        }
    }
}

void Sidebar::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Sidebar Background Panel & Right Border
    surface.fill_rect(abs_r, Color(0xFF0D1322));
    surface.draw_line(abs_r.right() - 1, abs_r.y, abs_r.right() - 1, abs_r.bottom(), Theme::Border());

    int32_t cur_y = abs_r.y + 10;
    int32_t row_w = (int32_t)abs_r.width - 16;

    for (size_t i = 0; i < m_item_count; ++i) {
        if (cur_y + (int32_t)m_item_height > abs_r.bottom() - 10) break;

        Rect row_rect(abs_r.x + 8, cur_y, (uint32_t)row_w, m_item_height);
        bool is_sel = ((int32_t)i == m_selected_index);
        bool is_hov = ((int32_t)i == m_hover_index);

        if (is_sel) {
            surface.fill_rounded_rect(row_rect, Theme::RadiusControl(), Color(0xFF1E293B));
            // Blue indicator capsule on the left
            Rect ind_rect(row_rect.x + 2, row_rect.y + 6, 3, row_rect.height - 12);
            surface.fill_rounded_rect(ind_rect, 1, Theme::Accent());
        } else if (is_hov) {
            surface.fill_rounded_rect(row_rect, Theme::RadiusControl(), Color(0xFF161F30));
        }

        const SidebarItem& it = m_items[i];
        int32_t icon_x = row_rect.x + 12;

        if (it.icon.is_valid()) {
            int32_t icon_y = row_rect.y + ((int32_t)m_item_height - (int32_t)it.icon.size().height) / 2;
            it.icon.draw(surface, Point(icon_x, icon_y));
            icon_x += it.icon.size().width + 10;
        }

        int32_t text_y = row_rect.y + ((int32_t)m_item_height - 16) / 2;
        Color text_col = is_sel ? Theme::TextPrimary() : (is_hov ? Theme::TextPrimary() : Theme::TextSecondary());
        surface.draw_string(icon_x, text_y, it.label, text_col);

        cur_y += m_item_height + 4;
    }

    Widget::paint(surface);
}

bool Sidebar::on_event(const Event& event) {
    if (!enabled()) return false;

    Rect abs_r = absolute_bounds();

    if (event.type == EventType::MouseMove) {
        int32_t rel_y = event.mouse_pos.y - (abs_r.y + 10);
        int32_t new_hover = -1;
        if (rel_y >= 0 && event.mouse_pos.x >= abs_r.x && event.mouse_pos.x <= abs_r.right()) {
            int32_t idx = rel_y / ((int32_t)m_item_height + 4);
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

    return Widget::on_event(event);
}

Size Sidebar::measure_preferred_size() const {
    return Size(180, 400);
}

} // namespace bos
