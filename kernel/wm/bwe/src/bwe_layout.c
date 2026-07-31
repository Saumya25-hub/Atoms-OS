#include "../include/bwe_layout.h"
#include "../include/bwe_geometry.h"
#include "../include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

// ============================================================
// Phase 1: Measure Pass
// ============================================================

void BWE_MeasureWindow(uint32_t window_id, int32_t avail_w, int32_t avail_h) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || win->state == BWE_STATE_HIDDEN) return;

    int32_t client_w = avail_w - win->margins.left - win->margins.right - win->padding.left - win->padding.right;
    int32_t client_h = avail_h - win->margins.top - win->margins.bottom - win->padding.top - win->padding.bottom;
    if (client_w < 0) client_w = 0;
    if (client_h < 0) client_h = 0;

    int32_t desired_w = 0;
    int32_t desired_h = 0;

    BWELayoutMode mode = win->layout_props.mode;

    if (mode == BWE_LAYOUT_STACK) {
        if (win->layout_props.orientation == BWE_ORIENT_VERTICAL) {
            for (uint32_t i = 0; i < win->child_count; i++) {
                BWE_Window* child = BWE_GetWindow(win->children[i]);
                if (!child || child->state == BWE_STATE_HIDDEN) continue;
                BWE_MeasureWindow(child->id, client_w, client_h);
                if (child->layout_props.desired_size.width > desired_w) {
                    desired_w = child->layout_props.desired_size.width;
                }
                desired_h += child->layout_props.desired_size.height + win->layout_props.spacing;
            }
            if (win->child_count > 0 && desired_h > 0) {
                desired_h -= win->layout_props.spacing; // Remove trailing spacing
            }
        } else { // BWE_ORIENT_HORIZONTAL
            for (uint32_t i = 0; i < win->child_count; i++) {
                BWE_Window* child = BWE_GetWindow(win->children[i]);
                if (!child || child->state == BWE_STATE_HIDDEN) continue;
                BWE_MeasureWindow(child->id, client_w, client_h);
                if (child->layout_props.desired_size.height > desired_h) {
                    desired_h = child->layout_props.desired_size.height;
                }
                desired_w += child->layout_props.desired_size.width + win->layout_props.spacing;
            }
            if (win->child_count > 0 && desired_w > 0) {
                desired_w -= win->layout_props.spacing;
            }
        }
    } else if (mode == BWE_LAYOUT_DOCK) {
        int32_t rem_w = client_w;
        int32_t rem_h = client_h;
        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;
            BWE_MeasureWindow(child->id, rem_w, rem_h);
            BWEDockPosition pos = child->layout_props.dock_pos;
            if (pos == BWE_DOCK_TOP || pos == BWE_DOCK_BOTTOM) {
                rem_h -= child->layout_props.desired_size.height;
            } else if (pos == BWE_DOCK_LEFT || pos == BWE_DOCK_RIGHT) {
                rem_w -= child->layout_props.desired_size.width;
            }
        }
        desired_w = client_w - rem_w;
        desired_h = client_h - rem_h;
    } else if (mode == BWE_LAYOUT_GRID) {
        // Measure children and assign track sizes
        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;
            BWE_MeasureWindow(child->id, client_w, client_h);
        }
        desired_w = client_w;
        desired_h = client_h;
    } else { // ABSOLUTE or DEFAULT
        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;
            BWE_MeasureWindow(child->id, client_w, client_h);
            int32_t right = child->local_bounds.x + child->local_bounds.width;
            int32_t bottom = child->local_bounds.y + child->local_bounds.height;
            if (right > desired_w) desired_w = right;
        }
    }
    if (desired_w < win->local_bounds.width) desired_w = win->local_bounds.width;
    if (desired_h < win->local_bounds.height) desired_h = win->local_bounds.height;

    desired_w += win->margins.left + win->margins.right + win->padding.left + win->padding.right;
    desired_h += win->margins.top + win->margins.bottom + win->padding.top + win->padding.bottom;

    if (win->min_size.width > 0 && desired_w < win->min_size.width) desired_w = win->min_size.width;
    if (win->min_size.height > 0 && desired_h < win->min_size.height) desired_h = win->min_size.height;
    if (win->max_size.width > 0 && desired_w > win->max_size.width) desired_w = win->max_size.width;
    if (win->max_size.height > 0 && desired_h > win->max_size.height) desired_h = win->max_size.height;

    win->layout_props.desired_size.width = desired_w;
    win->layout_props.desired_size.height = desired_h;
}

// ============================================================
// Phase 2: Arrange Pass
// ============================================================

void BWE_ArrangeWindow(uint32_t window_id, const BWE_Rect* final_rect) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || win->state == BWE_STATE_HIDDEN) return;

    if (final_rect) {
        win->local_bounds = *final_rect;
    }

    // Resolve screen_bounds relative to parent client
    BWE_Rect p_client;
    if (win->parent_id == BWE_DESKTOP_ID || win->parent_id == 0) {
        p_client = (BWE_Rect){0, 0, (int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height};
        win->screen_bounds = win->local_bounds;
    } else {
        BWE_Window* parent = BWE_GetWindow(win->parent_id);
        if (parent) {
            BWE_Geometry_CalculateClientBounds(parent, &p_client);
            win->screen_bounds.x = p_client.x + win->local_bounds.x;
            win->screen_bounds.y = p_client.y + win->local_bounds.y;
            win->screen_bounds.width = win->local_bounds.width;
            win->screen_bounds.height = win->local_bounds.height;
        }
    }

    // Client area available for children
    BWE_Rect inner_client;
    BWE_Geometry_CalculateClientBounds(win, &inner_client);

    BWELayoutMode mode = win->layout_props.mode;

    if (mode == BWE_LAYOUT_DOCK) {
        BWE_Rect rem = inner_client;

        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;

            BWEDockPosition dock = child->layout_props.dock_pos;
            BWE_Rect child_rect = {0};

            if (dock == BWE_DOCK_TOP) {
                int32_t h = child->layout_props.desired_size.height;
                if (h <= 0) h = child->local_bounds.height;
                if (h > rem.height) h = rem.height;
                child_rect = (BWE_Rect){rem.x - inner_client.x, rem.y - inner_client.y, rem.width, h};
                rem.y += h;
                rem.height -= h;
            } else if (dock == BWE_DOCK_BOTTOM) {
                int32_t h = child->layout_props.desired_size.height;
                if (h <= 0) h = child->local_bounds.height;
                if (h > rem.height) h = rem.height;
                child_rect = (BWE_Rect){rem.x - inner_client.x, (rem.y + rem.height - h) - inner_client.y, rem.width, h};
                rem.height -= h;
            } else if (dock == BWE_DOCK_LEFT) {
                int32_t w = child->layout_props.desired_size.width;
                if (w <= 0) w = child->local_bounds.width;
                if (w > rem.width) w = rem.width;
                child_rect = (BWE_Rect){rem.x - inner_client.x, rem.y - inner_client.y, w, rem.height};
                rem.x += w;
                rem.width -= w;
            } else if (dock == BWE_DOCK_RIGHT) {
                int32_t w = child->layout_props.desired_size.width;
                if (w <= 0) w = child->local_bounds.width;
                if (w > rem.width) w = rem.width;
                child_rect = (BWE_Rect){(rem.x + rem.width - w) - inner_client.x, rem.y - inner_client.y, w, rem.height};
                rem.width -= w;
            } else if (dock == BWE_DOCK_FILL) {
                child_rect = (BWE_Rect){rem.x - inner_client.x, rem.y - inner_client.y, rem.width, rem.height};
            } else {
                child_rect = child->local_bounds;
            }

            BWE_ArrangeWindow(child->id, &child_rect);
        }
    } else if (mode == BWE_LAYOUT_STACK) {
        int32_t cur_x = 0;
        int32_t cur_y = 0;
        int32_t spacing = win->layout_props.spacing;

        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;

            BWE_Rect child_rect = {0};
            if (win->layout_props.orientation == BWE_ORIENT_VERTICAL) {
                int32_t h = child->layout_props.desired_size.height;
                if (h <= 0) h = child->local_bounds.height;
                child_rect = (BWE_Rect){0, cur_y, inner_client.width, h};
                cur_y += h + spacing;
            } else { // HORIZONTAL
                int32_t w = child->layout_props.desired_size.width;
                if (w <= 0) w = child->local_bounds.width;
                child_rect = (BWE_Rect){cur_x, 0, w, inner_client.height};
                cur_x += w + spacing;
            }

            BWE_ArrangeWindow(child->id, &child_rect);
        }
    } else if (mode == BWE_LAYOUT_GRID) {
        uint32_t rows = win->layout_props.row_count > 0 ? win->layout_props.row_count : 1;
        uint32_t cols = win->layout_props.col_count > 0 ? win->layout_props.col_count : 1;

        int32_t cell_w = inner_client.width / cols;
        int32_t cell_h = inner_client.height / rows;

        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;

            uint32_t r = child->layout_props.grid_row;
            uint32_t c = child->layout_props.grid_col;
            uint32_t rspan = child->layout_props.grid_row_span > 0 ? child->layout_props.grid_row_span : 1;
            uint32_t cspan = child->layout_props.grid_col_span > 0 ? child->layout_props.grid_col_span : 1;

            BWE_Rect child_rect = (BWE_Rect){(int32_t)(c * cell_w), (int32_t)(r * cell_h), (int32_t)(cspan * cell_w), (int32_t)(rspan * cell_h)};
            BWE_ArrangeWindow(child->id, &child_rect);
        }
    } else { // ABSOLUTE or FALLBACK
        for (uint32_t i = 0; i < win->child_count; i++) {
            BWE_Window* child = BWE_GetWindow(win->children[i]);
            if (!child || child->state == BWE_STATE_HIDDEN) continue;
            BWE_ArrangeWindow(child->id, NULL);
        }
    }
}

// ============================================================
// Layout Invalidation & Update Dispatches
// ============================================================

void BWE_InvalidateLayout(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;

    win->layout_props.layout_dirty = true;
    win->is_dirty = true;

    // Bubble up layout invalidation to top-level window
    uint32_t curr = win->parent_id;
    while (curr != BWE_DESKTOP_ID && curr != 0) {
        BWE_Window* p = BWE_GetWindow(curr);
        if (p) {
            p->layout_props.layout_dirty = true;
            p->is_dirty = true;
            curr = p->parent_id;
        } else {
            break;
        }
    }

    BWE_UpdateLayout(window_id);
}

static const char* dock_name(BWEDockPosition pos) {
    switch (pos) {
        case BWE_DOCK_TOP: return "DOCK_TOP";
        case BWE_DOCK_BOTTOM: return "DOCK_BOTTOM";
        case BWE_DOCK_LEFT: return "DOCK_LEFT";
        case BWE_DOCK_RIGHT: return "DOCK_RIGHT";
        case BWE_DOCK_FILL: return "DOCK_FILL";
        default: return "DOCK_NONE";
    }
}

static const char* mode_name(BWELayoutMode mode) {
    switch (mode) {
        case BWE_LAYOUT_DOCK: return "DOCK";
        case BWE_LAYOUT_STACK: return "STACK";
        case BWE_LAYOUT_GRID: return "GRID";
        case BWE_LAYOUT_SPLIT: return "SPLIT";
        case BWE_LAYOUT_SCROLL: return "SCROLL";
        default: return "ABSOLUTE";
    }
}

void BWE_DumpLayoutTree(uint32_t window_id, int depth) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;

    extern void serial_write_direct(const char* str);
    extern void serial_write_dec_direct(int val);

    for (int i = 0; i < depth; i++) serial_write_direct("  ");
    serial_write_direct("├ [ID=");
    serial_write_dec_direct((int)win->id);
    serial_write_direct(" Parent=");
    serial_write_dec_direct((int)win->parent_id);
    serial_write_direct(" Type=");
    serial_write_dec_direct((int)win->type);
    serial_write_direct(" Mode=");
    serial_write_direct(mode_name(win->layout_props.mode));
    serial_write_direct(" Dock=");
    serial_write_direct(dock_name(win->layout_props.dock_pos));
    serial_write_direct(" Desired=(");
    serial_write_dec_direct(win->layout_props.desired_size.width);
    serial_write_direct("x");
    serial_write_dec_direct(win->layout_props.desired_size.height);
    serial_write_direct(") Local=(");
    serial_write_dec_direct(win->local_bounds.x);
    serial_write_direct(",");
    serial_write_dec_direct(win->local_bounds.y);
    serial_write_direct(" ");
    serial_write_dec_direct(win->local_bounds.width);
    serial_write_direct("x");
    serial_write_dec_direct(win->local_bounds.height);
    serial_write_direct(") Screen=(");
    serial_write_dec_direct(win->screen_bounds.x);
    serial_write_direct(",");
    serial_write_dec_direct(win->screen_bounds.y);
    serial_write_direct(" ");
    serial_write_dec_direct(win->screen_bounds.width);
    serial_write_direct("x");
    serial_write_dec_direct(win->screen_bounds.height);
    serial_write_direct(")]\n");

    for (uint32_t i = 0; i < win->child_count; i++) {
        BWE_DumpLayoutTree(win->children[i], depth + 1);
    }
}

void BWE_UpdateLayout(uint32_t parent_id) {
    BWE_Window* parent = BWE_GetWindow(parent_id);
    if (!parent) return;

    BWE_Rect p_client;
    if (parent_id == BWE_DESKTOP_ID || parent_id == 0) {
        p_client = (BWE_Rect){0, 0, (int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height};
    } else {
        BWE_Geometry_CalculateClientBounds(parent, &p_client);
    }

    // Run Two-Pass Layout Engine
    BWE_MeasureWindow(parent_id, p_client.width, p_client.height);
    BWE_ArrangeWindow(parent_id, NULL);

    extern void serial_write_direct(const char* str);
    serial_write_direct("[LAYOUT_TREE_DUMP START]\n");
    BWE_DumpLayoutTree(parent_id, 0);
    serial_write_direct("[LAYOUT_TREE_DUMP END]\n");
}

// ============================================================
// Container Creation APIs
// ============================================================

extern void bwe_panel_render(BWE_Window* self);
extern void bwe_panel_event(uint32_t window_id, const BWE_Event* event);

bwe_error_t BOS_CreateDockPanel(uint32_t parent_id, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, 0, 0, 100, 100, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PANEL;
        win->layout_props.mode = BWE_LAYOUT_DOCK;
        win->on_render = bwe_panel_render;
        win->on_event = bwe_panel_event;
    }
    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

bwe_error_t BOS_CreateStackPanel(uint32_t parent_id, BWEOrientation orientation, int32_t spacing, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, 0, 0, 100, 100, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PANEL;
        win->layout_props.mode = BWE_LAYOUT_STACK;
        win->layout_props.orientation = orientation;
        win->layout_props.spacing = spacing;
        win->on_render = bwe_panel_render;
        win->on_event = bwe_panel_event;
    }
    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

bwe_error_t BOS_CreateGridPanel(uint32_t parent_id, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, 0, 0, 100, 100, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PANEL;
        win->layout_props.mode = BWE_LAYOUT_GRID;
        win->on_render = bwe_panel_render;
        win->on_event = bwe_panel_event;
    }
    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

bwe_error_t BOS_CreateScrollViewer(uint32_t parent_id, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, 0, 0, 100, 100, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_PANEL;
        win->layout_props.mode = BWE_LAYOUT_SCROLL;
        win->on_render = bwe_panel_render;
        win->on_event = bwe_panel_event;
    }
    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

// ============================================================
// Configuration & Positioning APIs
// ============================================================

void BWE_SetDockPosition(uint32_t window_id, BWEDockPosition dock) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;
    win->layout_props.dock_pos = dock;
    BWE_InvalidateLayout(window_id);
}

void BWE_Grid_AddRow(uint32_t grid_id, BWLength height) {
    BWE_Window* win = BWE_GetWindow(grid_id);
    if (!win || win->layout_props.row_count >= BWE_MAX_GRID_ROWS) return;
    win->layout_props.rows[win->layout_props.row_count++].length = height;
    BWE_InvalidateLayout(grid_id);
}

void BWE_Grid_AddColumn(uint32_t grid_id, BWLength width) {
    BWE_Window* win = BWE_GetWindow(grid_id);
    if (!win || win->layout_props.col_count >= BWE_MAX_GRID_COLS) return;
    win->layout_props.cols[win->layout_props.col_count++].length = width;
    BWE_InvalidateLayout(grid_id);
}

void BWE_Grid_SetCell(uint32_t window_id, uint32_t row, uint32_t col) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;
    win->layout_props.grid_row = row;
    win->layout_props.grid_col = col;
    BWE_InvalidateLayout(window_id);
}

void BWE_Grid_SetCellSpan(uint32_t window_id, uint32_t row_span, uint32_t col_span) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;
    win->layout_props.grid_row_span = row_span;
    win->layout_props.grid_col_span = col_span;
    BWE_InvalidateLayout(window_id);
}

// ============================================================
// Legacy Helpers & Maximize Operations
// ============================================================

void BWE_SetAnchorMode(uint32_t window_id, uint8_t anchor_flags) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return;
    win->anchor_flags = anchor_flags;
    if (win->parent_id != BWE_DESKTOP_ID && win->parent_id != 0) {
        BWE_UpdateLayout(win->parent_id);
    }
}

bool BWE_WindowIsMaximized(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win) return false;
    return (win->flags & BWE_WINDOW_FULLSCREEN) != 0;
}

void BWE_WindowMaximize(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || BWE_WindowIsMaximized(window_id)) return;

    if (win->parent_id == BWE_DESKTOP_ID || win->parent_id == 0) {
        win->restore_bounds = win->screen_bounds;
        win->flags |= BWE_WINDOW_FULLSCREEN;
        BOS_SetBounds(window_id, 0, 0, (int32_t)g_kernel_screen_width, (int32_t)g_kernel_screen_height - 32);
    }
}

void BWE_WindowRestore(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || !BWE_WindowIsMaximized(window_id)) return;

    win->flags &= ~BWE_WINDOW_FULLSCREEN;
    BOS_SetBounds(window_id, win->restore_bounds.x, win->restore_bounds.y, win->restore_bounds.width, win->restore_bounds.height);
}
