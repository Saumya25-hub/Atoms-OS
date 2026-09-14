#ifndef BOS_UI_LAYOUT_HPP
#define BOS_UI_LAYOUT_HPP

#include "widget.hpp"

namespace bos {

// ============================================================================
// Layout Engine Base Interface
// ============================================================================
class Layout {
public:
    virtual ~Layout() = default;
    virtual void apply(Widget& container) = 0;
};

// ============================================================================
// Manual Layout (Absolute Positioning)
// ============================================================================
class ManualLayout : public Layout {
public:
    void apply(Widget& container) override {
        (void)container;
    }
};

// ============================================================================
// Stack Layout (Children fill container content bounds)
// ============================================================================
class StackLayout : public Layout {
public:
    void apply(Widget& container) override {
        Rect content = container.content_bounds();
        for (size_t i = 0; i < container.child_count(); ++i) {
            Widget* child = container.child_at(i);
            if (child && child->visible()) {
                Insets m = child->margin();
                Rect target(content.x + m.left,
                            content.y + m.top,
                            (content.width > (uint32_t)m.horizontal()) ? content.width - m.horizontal() : 0,
                            (content.height > (uint32_t)m.vertical()) ? content.height - m.vertical() : 0);
                child->set_bounds(target);
            }
        }
    }
};

// ============================================================================
// Linear Layout (HBox / VBox with Spacing, Margins, and Alignment)
// ============================================================================
class LinearLayout : public Layout {
public:
    LinearLayout(Orientation orientation = Orientation::Vertical,
                 int32_t spacing = 8,
                 Alignment cross_align = Alignment::Stretch)
        : m_orientation(orientation),
          m_spacing(spacing),
          m_alignment(cross_align) {}

    void set_orientation(Orientation o) { m_orientation = o; }
    Orientation orientation() const { return m_orientation; }

    void set_spacing(int32_t spacing) { m_spacing = spacing; }
    int32_t spacing() const { return m_spacing; }

    void set_alignment(Alignment align) { m_alignment = align; }
    Alignment alignment() const { return m_alignment; }

    void apply(Widget& container) override {
        Rect content = container.content_bounds();
        if (content.is_empty()) return;

        if (m_orientation == Orientation::Vertical) {
            int32_t cur_y = content.y;

            for (size_t i = 0; i < container.child_count(); ++i) {
                Widget* child = container.child_at(i);
                if (!child || !child->visible()) continue;

                Insets m = child->margin();
                Size pref = child->measure_preferred_size();
                uint32_t child_h = (pref.height > 0) ? pref.height : child->bounds().height;
                if (child_h == 0) child_h = 30;

                int32_t child_x = content.x + m.left;
                uint32_t child_w = 0;

                if (m_alignment == Alignment::Stretch) {
                    child_w = (content.width > (uint32_t)m.horizontal()) ? (content.width - m.horizontal()) : 0;
                } else {
                    child_w = (pref.width > 0) ? pref.width : child->bounds().width;
                    if (child_w > content.width - m.horizontal()) {
                        child_w = content.width - m.horizontal();
                    }
                    if (m_alignment == Alignment::Center) {
                        child_x = content.x + m.left + ((int32_t)(content.width - m.horizontal()) - (int32_t)child_w) / 2;
                    } else if (m_alignment == Alignment::End) {
                        child_x = content.right() - m.right - (int32_t)child_w;
                    }
                }

                child->set_bounds(Rect(child_x, cur_y + m.top, child_w, child_h));
                cur_y += child_h + m.vertical() + m_spacing;
            }
        } else {
            // Horizontal layout
            int32_t cur_x = content.x;

            for (size_t i = 0; i < container.child_count(); ++i) {
                Widget* child = container.child_at(i);
                if (!child || !child->visible()) continue;

                Insets m = child->margin();
                Size pref = child->measure_preferred_size();
                uint32_t child_w = (pref.width > 0) ? pref.width : child->bounds().width;
                if (child_w == 0) child_w = 80;

                int32_t child_y = content.y + m.top;
                uint32_t child_h = 0;

                if (m_alignment == Alignment::Stretch) {
                    child_h = (content.height > (uint32_t)m.vertical()) ? (content.height - m.vertical()) : 0;
                } else {
                    child_h = (pref.height > 0) ? pref.height : child->bounds().height;
                    if (child_h > content.height - m.vertical()) {
                        child_h = content.height - m.vertical();
                    }
                    if (m_alignment == Alignment::Center) {
                        child_y = content.y + m.top + ((int32_t)(content.height - m.vertical()) - (int32_t)child_h) / 2;
                    } else if (m_alignment == Alignment::End) {
                        child_y = content.bottom() - m.bottom - (int32_t)child_h;
                    }
                }

                child->set_bounds(Rect(cur_x + m.left, child_y, child_w, child_h));
                cur_x += child_w + m.horizontal() + m_spacing;
            }
        }
    }

private:
    Orientation m_orientation{Orientation::Vertical};
    int32_t     m_spacing{8};
    Alignment   m_alignment{Alignment::Stretch};
};

// ============================================================================
// Dock Anchor Enums
// ============================================================================
enum class DockEdge : uint32_t {
    Top = 0,
    Bottom,
    Left,
    Right,
    Fill
};

// ============================================================================
// Dock / Anchor Layout (Responsive Shell Pattern)
// ============================================================================
class AnchorLayout : public Layout {
public:
    static constexpr size_t MAX_DOCK_ITEMS = 16;

    struct DockEntry {
        Widget*  widget{nullptr};
        DockEdge edge{DockEdge::Fill};
        uint32_t extent{0}; // Height for Top/Bottom, Width for Left/Right
    };

    void add_dock(Widget* widget, DockEdge edge, uint32_t extent = 0) {
        if (!widget || m_count >= MAX_DOCK_ITEMS) return;
        m_entries[m_count++] = {widget, edge, extent};
    }

    void apply(Widget& container) override {
        Rect remaining = container.content_bounds();

        for (size_t i = 0; i < m_count; ++i) {
            Widget* w = m_entries[i].widget;
            if (!w || !w->visible()) continue;

            Insets m = w->margin();
            switch (m_entries[i].edge) {
                case DockEdge::Top: {
                    uint32_t h = (m_entries[i].extent > 0) ? m_entries[i].extent : w->bounds().height;
                    w->set_bounds(Rect(remaining.x + m.left, remaining.y + m.top,
                                       remaining.width - m.horizontal(), h));
                    remaining.y += h + m.vertical();
                    remaining.height = (remaining.height > h + m.vertical()) ? remaining.height - (h + m.vertical()) : 0;
                    break;
                }
                case DockEdge::Bottom: {
                    uint32_t h = (m_entries[i].extent > 0) ? m_entries[i].extent : w->bounds().height;
                    int32_t top_pos = remaining.bottom() - h - m.bottom;
                    w->set_bounds(Rect(remaining.x + m.left, top_pos,
                                       remaining.width - m.horizontal(), h));
                    remaining.height = (remaining.height > h + m.vertical()) ? remaining.height - (h + m.vertical()) : 0;
                    break;
                }
                case DockEdge::Left: {
                    uint32_t wd = (m_entries[i].extent > 0) ? m_entries[i].extent : w->bounds().width;
                    w->set_bounds(Rect(remaining.x + m.left, remaining.y + m.top,
                                       wd, remaining.height - m.vertical()));
                    remaining.x += wd + m.horizontal();
                    remaining.width = (remaining.width > wd + m.horizontal()) ? remaining.width - (wd + m.horizontal()) : 0;
                    break;
                }
                case DockEdge::Right: {
                    uint32_t wd = (m_entries[i].extent > 0) ? m_entries[i].extent : w->bounds().width;
                    int32_t left_pos = remaining.right() - wd - m.right;
                    w->set_bounds(Rect(left_pos, remaining.y + m.top,
                                       wd, remaining.height - m.vertical()));
                    remaining.width = (remaining.width > wd + m.horizontal()) ? remaining.width - (wd + m.horizontal()) : 0;
                    break;
                }
                case DockEdge::Fill: {
                    w->set_bounds(Rect(remaining.x + m.left, remaining.y + m.top,
                                       (remaining.width > (uint32_t)m.horizontal()) ? remaining.width - m.horizontal() : 0,
                                       (remaining.height > (uint32_t)m.vertical()) ? remaining.height - m.vertical() : 0));
                    break;
                }
            }
        }
    }

private:
    DockEntry m_entries[MAX_DOCK_ITEMS]{};
    size_t    m_count{0};
};

// ============================================================================
// Grid Layout (Rows x Columns with Spacing)
// ============================================================================
class GridLayout : public Layout {
public:
    GridLayout(uint32_t rows, uint32_t cols, int32_t h_gap = 8, int32_t v_gap = 8)
        : m_rows((rows == 0) ? 1 : rows),
          m_cols((cols == 0) ? 1 : cols),
          m_h_gap(h_gap),
          m_v_gap(v_gap) {}

    void apply(Widget& container) override {
        Rect content = container.content_bounds();
        if (content.is_empty() || m_rows == 0 || m_cols == 0) return;

        int32_t total_h_gaps = (int32_t)(m_cols - 1) * m_h_gap;
        int32_t total_v_gaps = (int32_t)(m_rows - 1) * m_v_gap;

        uint32_t cell_w = (content.width > (uint32_t)total_h_gaps) ? (content.width - total_h_gaps) / m_cols : 0;
        uint32_t cell_h = (content.height > (uint32_t)total_v_gaps) ? (content.height - total_v_gaps) / m_rows : 0;

        size_t child_idx = 0;
        for (uint32_t r = 0; r < m_rows; ++r) {
            for (uint32_t c = 0; c < m_cols; ++c) {
                if (child_idx >= container.child_count()) return;

                Widget* child = container.child_at(child_idx++);
                while (child && !child->visible() && child_idx < container.child_count()) {
                    child = container.child_at(child_idx++);
                }
                if (!child || !child->visible()) continue;

                Insets m = child->margin();
                int32_t cell_x = content.x + (int32_t)c * (cell_w + m_h_gap);
                int32_t cell_y = content.y + (int32_t)r * (cell_h + m_v_gap);

                child->set_bounds(Rect(cell_x + m.left, cell_y + m.top,
                                       (cell_w > (uint32_t)m.horizontal()) ? cell_w - m.horizontal() : 0,
                                       (cell_h > (uint32_t)m.vertical()) ? cell_h - m.vertical() : 0));
            }
        }
    }

private:
    uint32_t m_rows{1};
    uint32_t m_cols{1};
    int32_t  m_h_gap{8};
    int32_t  m_v_gap{8};
};

} // namespace bos

#endif // BOS_UI_LAYOUT_HPP
