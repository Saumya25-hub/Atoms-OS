#include "bos/controls/tabcontrol.hpp"
#include <string.h>

namespace bos {

TabControl::TabControl() {
    set_size(280, 200);
}

bool TabControl::add_tab(const char* title, Widget* content) {
    if (m_tab_count >= MAX_TABS || !title) return false;

    TabPage& t = m_tabs[m_tab_count++];
    strncpy(t.title, title, sizeof(t.title) - 1);
    t.title[sizeof(t.title) - 1] = '\0';
    t.content = content;

    if (content) {
        add_child(content);
        content->set_visible(m_tab_count - 1 == (size_t)m_active_tab);
        content->set_position(4, m_header_height + 4);
        content->set_size(bounds().width - 8, bounds().height - m_header_height - 8);
    }

    invalidate();
    return true;
}

void TabControl::clear_tabs() {
    m_tab_count = 0;
    m_active_tab = 0;
    m_hover_tab = -1;
    invalidate();
}

void TabControl::set_active_tab(int32_t index) {
    if (index >= 0 && index < (int32_t)m_tab_count && m_active_tab != index) {
        m_active_tab = index;

        // Update child widget visibility
        for (size_t i = 0; i < m_tab_count; ++i) {
            if (m_tabs[i].content) {
                m_tabs[i].content->set_visible((int32_t)i == m_active_tab);
            }
        }

        invalidate();
        if (m_on_tab_changed) {
            m_on_tab_changed(this, m_active_tab, m_user_data);
        }
    }
}

void TabControl::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Container Background & Border
    surface.fill_rounded_rect(abs_r, Theme::RadiusCard(), Theme::SurfaceCard());
    surface.draw_rounded_rect(abs_r, Theme::RadiusCard(), Theme::Border(), 1);

    // 2. Header Bar
    Rect header_bar(abs_r.x, abs_r.y, abs_r.width, m_header_height);
    surface.draw_line(abs_r.x, header_bar.bottom(), abs_r.right(), header_bar.bottom(), Theme::Border());

    int32_t cur_x = abs_r.x + 8;
    for (size_t i = 0; i < m_tab_count; ++i) {
        Size txt_sz = surface.measure_string(m_tabs[i].title);
        uint32_t tab_w = txt_sz.width + 20;

        Rect tab_rect(cur_x, abs_r.y + 4, tab_w, m_header_height - 8);
        bool is_act = ((int32_t)i == m_active_tab);
        bool is_hov = ((int32_t)i == m_hover_tab);

        if (is_act) {
            surface.fill_rounded_rect(tab_rect, Theme::RadiusSmall(), Theme::SurfaceSubtle());
            // Blue bottom highlight bar
            Rect btm_line(tab_rect.x + 4, header_bar.bottom() - 2, tab_rect.width - 8, 2);
            surface.fill_rect(btm_line, Theme::Accent());
        } else if (is_hov) {
            surface.fill_rounded_rect(tab_rect, Theme::RadiusSmall(), Color(0xFF161F30));
        }

        int32_t ty = tab_rect.y + ((int32_t)tab_rect.height - 16) / 2;
        int32_t tx = tab_rect.x + ((int32_t)tab_rect.width - (int32_t)txt_sz.width) / 2;
        Color col = is_act ? Theme::TextPrimary() : (is_hov ? Theme::TextPrimary() : Theme::TextSecondary());
        surface.draw_string(tx, ty, m_tabs[i].title, col);

        cur_x += tab_w + 6;
    }

    Widget::paint(surface);
}

bool TabControl::on_event(const Event& event) {
    if (!enabled()) return false;

    Rect abs_r = absolute_bounds();

    if (event.type == EventType::MouseMove) {
        if (event.mouse_pos.y >= abs_r.y && event.mouse_pos.y < abs_r.y + (int32_t)m_header_height) {
            int32_t cur_x = abs_r.x + 8;
            int32_t new_hover = -1;

            for (size_t i = 0; i < m_tab_count; ++i) {
                uint32_t len = (uint32_t)strlen(m_tabs[i].title);
                uint32_t tab_w = len * 8 + 20;
                if (event.mouse_pos.x >= cur_x && event.mouse_pos.x <= cur_x + (int32_t)tab_w) {
                    new_hover = (int32_t)i;
                    break;
                }
                cur_x += tab_w + 6;
            }

            if (new_hover != m_hover_tab) {
                m_hover_tab = new_hover;
                invalidate();
            }
            return true;
        } else if (m_hover_tab != -1) {
            m_hover_tab = -1;
            invalidate();
        }
    }

    if (event.type == EventType::MouseLeave) {
        if (m_hover_tab != -1) {
            m_hover_tab = -1;
            invalidate();
        }
    }

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        if (m_hover_tab >= 0 && m_hover_tab < (int32_t)m_tab_count) {
            set_active_tab(m_hover_tab);
            return true;
        }
    }

    return Widget::on_event(event);
}

Size TabControl::measure_preferred_size() const {
    return Size(280, 200);
}

} // namespace bos
