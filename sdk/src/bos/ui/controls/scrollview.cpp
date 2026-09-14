#include "bos/controls/scrollview.hpp"

namespace bos {

ScrollView::ScrollView() {
    set_size(240, 160);
}

void ScrollView::set_scroll_y(int32_t y) {
    int32_t max_scroll = m_content_height - (int32_t)bounds().height;
    if (max_scroll < 0) max_scroll = 0;

    if (y < 0) y = 0;
    if (y > max_scroll) y = max_scroll;

    if (m_scroll_y != y) {
        m_scroll_y = y;
        invalidate();
    }
}

void ScrollView::paint(Surface& surface) {
    if (!visible()) return;

    Rect abs_r = absolute_bounds();

    // 1. Background & Border
    surface.fill_rounded_rect(abs_r, Theme::RadiusControl(), Theme::SurfaceSubtle());
    surface.draw_rounded_rect(abs_r, Theme::RadiusControl(), Theme::Border(), 1);

    // 2. Viewport Clipping
    Rect prev_clip = surface.clip();
    Rect viewport = abs_r.inset(Insets(2, 2, 8, 2)); // leave room for scrollbar
    surface.set_clip(prev_clip.intersect(viewport));

    // Paint child widgets shifted by scroll offset
    for (size_t i = 0; i < m_child_count; ++i) {
        Widget* child = m_children[i];
        if (child && child->visible()) {
            Point orig_pos = child->position();
            child->set_position(orig_pos.x, orig_pos.y - m_scroll_y);
            child->paint(surface);
            child->set_position(orig_pos.x, orig_pos.y);
        }
    }

    surface.set_clip(prev_clip);

    // 3. Draw Vertical Scrollbar if content exceeds viewport
    int32_t view_h = (int32_t)abs_r.height;
    if (m_content_height > view_h) {
        int32_t bar_w = 6;
        int32_t bar_x = abs_r.right() - bar_w - 2;
        Rect bar_track(bar_x, abs_r.y + 2, (uint32_t)bar_w, (uint32_t)(abs_r.height - 4));
        surface.fill_rounded_rect(bar_track, 3, Color(0xFF1E293B));

        // Thumb size proportional to view/content
        int32_t thumb_h = (int32_t)((int64_t)view_h * (abs_r.height - 4) / m_content_height);
        if (thumb_h < 16) thumb_h = 16;

        int32_t max_scroll = m_content_height - view_h;
        int32_t thumb_y = bar_track.y + (int32_t)((int64_t)m_scroll_y * (bar_track.height - thumb_h) / max_scroll);

        Rect thumb_rect(bar_x, thumb_y, (uint32_t)bar_w, (uint32_t)thumb_h);
        Color thumb_color = m_dragging_thumb ? Theme::Accent() : Theme::BorderHighlight();
        surface.fill_rounded_rect(thumb_rect, 3, thumb_color);
    }
}

bool ScrollView::on_event(const Event& event) {
    if (!enabled()) return false;

    Rect abs_r = absolute_bounds();

    if (event.type == EventType::MouseDown && event.mouse_button == MouseButton::Left) {
        int32_t bar_x = abs_r.right() - 8;
        if (event.mouse_pos.x >= bar_x) {
            m_dragging_thumb = true;
            m_drag_start_y = event.mouse_pos.y;
            m_drag_start_scroll = m_scroll_y;
            invalidate();
            return true;
        }
    }

    if (event.type == EventType::MouseUp && event.mouse_button == MouseButton::Left) {
        if (m_dragging_thumb) {
            m_dragging_thumb = false;
            invalidate();
            return true;
        }
    }

    if (event.type == EventType::MouseMove && m_dragging_thumb) {
        int32_t dy = event.mouse_pos.y - m_drag_start_y;
        int32_t max_scroll = m_content_height - (int32_t)abs_r.height;
        if (max_scroll > 0) {
            int32_t scroll_delta = (int32_t)((int64_t)dy * m_content_height / abs_r.height);
            set_scroll_y(m_drag_start_scroll + scroll_delta);
        }
        return true;
    }

    if (event.type == EventType::MouseWheel) {
        int32_t delta = (event.wheel_dy != 0) ? (-event.wheel_dy * 24) : 24;
        set_scroll_y(m_scroll_y + delta);
        return true;
    }

    return Widget::on_event(event);
}

Size ScrollView::measure_preferred_size() const {
    return Size(240, 160);
}

} // namespace bos
