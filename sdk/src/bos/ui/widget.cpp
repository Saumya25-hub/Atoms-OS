#include "bos/widget.hpp"
#include "bos/window.hpp"
#include "bos/layout.hpp"

namespace bos {

Widget::Widget() = default;

Widget::~Widget() {
    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i]) {
            m_children[i]->m_parent = nullptr;
            m_children[i] = nullptr;
        }
    }
    m_child_count = 0;
    m_parent = nullptr;
}

Window* Widget::root_window() const {
    const Widget* cur = this;
    while (cur->m_parent != nullptr) {
        cur = cur->m_parent;
    }
    // The top-level widget is owned by a Window; Window exposes itself
    return reinterpret_cast<Window*>(const_cast<Widget*>(cur)->m_parent);
}

bool Widget::add_child(Widget* child) {
    if (!child || child == this || m_child_count >= MAX_CHILDREN) return false;

    // Check if already a child
    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] == child) return true;
    }

    if (child->m_parent) {
        child->m_parent->remove_child(child);
    }

    m_children[m_child_count++] = child;
    child->m_parent = this;
    m_layout_dirty = true;
    perform_layout();
    child->invalidate();
    return true;
}

bool Widget::remove_child(Widget* child) {
    if (!child) return false;

    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] == child) {
            child->invalidate();
            child->m_parent = nullptr;

            for (size_t j = i; j < m_child_count - 1; ++j) {
                m_children[j] = m_children[j + 1];
            }
            m_children[--m_child_count] = nullptr;
            m_layout_dirty = true;
            perform_layout();
            return true;
        }
    }
    return false;
}

void Widget::set_bounds(const Rect& bounds) {
    if (m_bounds != bounds) {
        invalidate(); // invalidate old area
        m_bounds = bounds;
        m_layout_dirty = true;
        perform_layout();
        invalidate(); // invalidate new area
    }
}

void Widget::set_position(int32_t x, int32_t y) {
    set_bounds(Rect(x, y, m_bounds.width, m_bounds.height));
}

void Widget::set_size(uint32_t width, uint32_t height) {
    set_bounds(Rect(m_bounds.x, m_bounds.y, width, height));
}

void Widget::set_layout(Layout* layout) {
    m_layout = layout;
    m_layout_dirty = true;
    perform_layout();
}

void Widget::perform_layout() {
    if (m_layout) {
        m_layout->apply(*this);
        m_layout_dirty = false;
    }

    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] && m_children[i]->visible()) {
            m_children[i]->perform_layout();
        }
    }
}

void Widget::invalidate_layout() {
    m_layout_dirty = true;
    if (m_parent) {
        m_parent->invalidate_layout();
    }
}

bool Widget::has_mouse_capture() const {
    Window* win = root_window();
    return win ? (win->mouse_capture() == this) : false;
}

void Widget::set_mouse_capture() {
    Window* win = root_window();
    if (win) {
        win->set_mouse_capture(this);
    }
}

void Widget::release_mouse_capture() {
    Window* win = root_window();
    if (win) {
        win->release_mouse_capture();
    }
}

Rect Widget::absolute_bounds() const {
    Point origin = local_to_window(Point(0, 0));
    return Rect(origin.x, origin.y, m_bounds.width, m_bounds.height);
}

Point Widget::local_to_window(Point p) const {
    Point pt = p + m_bounds.origin();
    if (m_parent) {
        return m_parent->local_to_window(pt);
    }
    return pt;
}

Point Widget::window_to_local(Point p) const {
    Point origin = local_to_window(Point(0, 0));
    return p - origin;
}

void Widget::set_visible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        if (m_parent) {
            m_parent->mark_layout_dirty();
            m_parent->perform_layout();
        }
        invalidate();
    }
}

void Widget::set_enabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        invalidate();
    }
}

void Widget::set_hovered(bool hovered) {
    if (m_is_hovered != hovered) {
        m_is_hovered = hovered;
        if (m_is_hovered) {
            on_mouse_enter();
        } else {
            on_mouse_leave();
        }
        invalidate();
    }
}

void Widget::set_focused(bool focused) {
    if (m_is_focused != focused) {
        m_is_focused = focused;
        if (m_is_focused) {
            on_focus_gained();
        } else {
            on_focus_lost();
        }
        invalidate();
    }
}

Widget* Widget::hit_test(Point window_pt) {
    if (!m_visible || !m_enabled) return nullptr;

    Rect abs_r = absolute_bounds();
    if (!abs_r.contains(window_pt)) return nullptr;

    // Check children in reverse order (topmost first)
    for (size_t i = m_child_count; i > 0; --i) {
        Widget* child = m_children[i - 1];
        if (child) {
            Widget* hit = child->hit_test(window_pt);
            if (hit) return hit;
        }
    }

    return this;
}

void Widget::invalidate() {
    invalidate(Rect(0, 0, m_bounds.width, m_bounds.height));
}

void Widget::invalidate(const Rect& local_rect) {
    if (!m_visible || local_rect.is_empty()) return;

    if (m_dirty_rect.is_empty()) {
        m_dirty_rect = local_rect;
    } else {
        int32_t nx = (m_dirty_rect.left() < local_rect.left()) ? m_dirty_rect.left() : local_rect.left();
        int32_t ny = (m_dirty_rect.top() < local_rect.top()) ? m_dirty_rect.top() : local_rect.top();
        int32_t nr = (m_dirty_rect.right() > local_rect.right()) ? m_dirty_rect.right() : local_rect.right();
        int32_t nb = (m_dirty_rect.bottom() > local_rect.bottom()) ? m_dirty_rect.bottom() : local_rect.bottom();
        m_dirty_rect = Rect(nx, ny, (uint32_t)(nr - nx), (uint32_t)(nb - ny));
    }

    Point abs_origin = local_to_window(local_rect.origin());
    Rect win_dirty(abs_origin.x, abs_origin.y, local_rect.width, local_rect.height);

    // Bubble up to parent until root
    Widget* cur = this;
    while (cur->m_parent != nullptr) {
        cur = cur->m_parent;
    }

    // Root widget's parent pointer in our architecture holds the Window instance
    Window* win = reinterpret_cast<Window*>(cur->m_parent);
    if (win) {
        win->invalidate(win_dirty);
    }
}

void Widget::paint(Surface& surface) {
    if (!m_visible) return;

    Rect abs_r = absolute_bounds();
    Rect prev_clip = surface.clip();
    surface.set_clip(prev_clip.intersect(abs_r));

    // Paint children clipped to content_bounds()
    Point content_origin = local_to_window(content_bounds().origin());
    Rect content_abs(content_origin.x, content_origin.y, content_bounds().width, content_bounds().height);
    Rect child_clip = prev_clip.intersect(content_abs);
    surface.set_clip(child_clip);

    for (size_t i = 0; i < m_child_count; ++i) {
        if (m_children[i] && m_children[i]->visible()) {
            m_children[i]->paint(surface);
        }
    }

    surface.set_clip(prev_clip);
}

bool Widget::on_event(const Event& event) {
    (void)event;
    return false;
}

void Widget::on_mouse_enter() {}
void Widget::on_mouse_leave() {}
void Widget::on_focus_gained() {}
void Widget::on_focus_lost() {}

Size Widget::measure_preferred_size() const {
    return m_bounds.size();
}

} // namespace bos
