#ifndef BOS_UI_WIDGET_HPP
#define BOS_UI_WIDGET_HPP

#include <stdint.h>
#include <stddef.h>
#include "types.hpp"
#include "geometry.hpp"
#include "events.hpp"
#include "surface.hpp"

namespace bos {

class Window;
class Layout;

// ============================================================================
// Base Widget Class (Phase 4 Event Routing, Layout, Margin/Padding, Focus)
// ============================================================================
class Widget {
public:
    static constexpr size_t MAX_CHILDREN = 32;

    Widget();
    virtual ~Widget();

    // Non-copyable
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // Hierarchy
    Widget* parent() const { return m_parent; }
    Window* root_window() const;
    size_t child_count() const { return m_child_count; }
    Widget* child_at(size_t index) const { return (index < m_child_count) ? m_children[index] : nullptr; }

    bool add_child(Widget* child);
    bool remove_child(Widget* child);

    // Geometry
    Rect bounds() const { return m_bounds; }
    void set_bounds(const Rect& bounds);
    void set_position(int32_t x, int32_t y);
    void set_size(uint32_t width, uint32_t height);

    Point position() const { return m_bounds.origin(); }
    Size size() const { return m_bounds.size(); }

    Rect absolute_bounds() const;
    Point local_to_window(Point p) const;
    Point window_to_local(Point p) const;

    // Margins, Padding, and Content Bounds (Phase 4)
    Insets margin() const { return m_margin; }
    void set_margin(const Insets& m) { m_margin = m; invalidate(); }

    Insets padding() const { return m_padding; }
    void set_padding(const Insets& p) { m_padding = p; invalidate(); }

    Rect content_bounds() const {
        return Rect(0, 0, m_bounds.width, m_bounds.height).inset(m_padding);
    }

    // Size Constraints
    Size min_size() const { return m_min_size; }
    void set_min_size(Size s) { m_min_size = s; }

    Size max_size() const { return m_max_size; }
    void set_max_size(Size s) { m_max_size = s; }

    // Layout Engine Integration (Phase 4)
    Layout* layout() const { return m_layout; }
    void set_layout(Layout* layout);
    void perform_layout();
    void mark_layout_dirty() { m_layout_dirty = true; }
    void invalidate_layout();
    Size preferred_size() const { return measure_preferred_size(); }

    // State
    bool visible() const { return m_visible; }
    void set_visible(bool visible);

    bool enabled() const { return m_enabled; }
    void set_enabled(bool enabled);

    bool focusable() const { return m_focusable; }
    void set_focusable(bool focusable) { m_focusable = focusable; }

    bool is_focused() const { return m_is_focused; }
    bool is_hovered() const { return m_is_hovered; }

    // Tab Navigation Order
    int32_t tab_index() const { return m_tab_index; }
    void set_tab_index(int32_t index) { m_tab_index = index; }

    // Mouse Capture
    bool has_mouse_capture() const;
    void set_mouse_capture();
    void release_mouse_capture();

    // Hit Testing & Invalidation
    Widget* hit_test(Point window_pt);
    void invalidate();
    void invalidate(const Rect& local_rect);

    // Dirty Rect Tracking
    bool is_dirty() const { return !m_dirty_rect.is_empty(); }
    const Rect& dirty_rect() const { return m_dirty_rect; }
    void clear_dirty() { m_dirty_rect = Rect(0, 0, 0, 0); }

    // Rendering Hook (Override in concrete controls)
    virtual void paint(Surface& surface);

    // Event Hooks
    virtual bool on_event(const Event& event);
    virtual void on_mouse_enter();
    virtual void on_mouse_leave();
    virtual void on_focus_gained();
    virtual void on_focus_lost();
    virtual Size measure_preferred_size() const;

protected:
    friend class Window;
    void set_parent(Widget* parent) { m_parent = parent; }
    void set_hovered(bool hovered);
    void set_focused(bool focused);

    Widget*   m_parent{nullptr};
    Widget*   m_children[MAX_CHILDREN]{nullptr};
    size_t    m_child_count{0};

    Rect      m_bounds{0, 0, 100, 30};
    Insets    m_margin{0, 0, 0, 0};
    Insets    m_padding{0, 0, 0, 0};
    Size      m_min_size{0, 0};
    Size      m_max_size{0xFFFFFFFF, 0xFFFFFFFF};
    Layout*   m_layout{nullptr};
    bool      m_layout_dirty{true};

    int32_t   m_tab_index{0};
    Rect      m_dirty_rect{0, 0, 0, 0};

    bool      m_visible{true};
    bool      m_enabled{true};
    bool      m_focusable{false};
    bool      m_is_focused{false};
    bool      m_is_hovered{false};
};

} // namespace bos

#endif // BOS_UI_WIDGET_HPP
