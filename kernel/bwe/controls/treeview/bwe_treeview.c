#include "../../include/bwe.h"

// Expose internal Z-order stack or window pool
extern BWE_Window* BWE_GetWindow(uint32_t window_id);

static void bwe_treeview_render(BWE_Window* self) {
    extern void BWE_FillRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    extern void BWE_DrawRect(const BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t thickness);
    extern void BWE_DrawText(const BVFramebuffer* fb, const char* text, int32_t x, int32_t y, uint32_t color, BWE_Font* font);
    extern const BVFramebuffer* BWE_GetRenderTarget(void);

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    // Fill white box background
    BWE_FillRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, 0xFFFFFFFF);
    
    // Border
    extern uint32_t g_focused_window_id;
    uint32_t border_color = (self->id == g_focused_window_id) ? BWE_ThemeGetColor(BWE_THEME_WINDOW_BORDER_ACTIVE) : BWE_ThemeGetColor(BWE_THEME_CONTROL_BORDER);
    BWE_DrawRect(fb, self->screen_bounds.x, self->screen_bounds.y, self->screen_bounds.width, self->screen_bounds.height, border_color, 1);

    uint32_t count = self->control_data.treeview.node_count;
    int32_t selected = self->control_data.treeview.selected_index;
    int32_t item_height = 20;
    int32_t indent_step = 16;

    int32_t current_y = self->screen_bounds.y + 2;

    for (uint32_t i = 0; i < count; i++) {
        // Calculate tree level depth by walking parents
        int32_t depth = 0;
        int32_t parent_idx = self->control_data.treeview.parent_node_index[i];
        while (parent_idx != -1) {
            depth++;
            parent_idx = self->control_data.treeview.parent_node_index[parent_idx];
        }

        // If parent is collapsed, we skip rendering this node!
        bool parent_expanded = true;
        int32_t p_idx = self->control_data.treeview.parent_node_index[i];
        while (p_idx != -1) {
            if (!self->control_data.treeview.expanded[p_idx]) {
                parent_expanded = false;
                break;
            }
            p_idx = self->control_data.treeview.parent_node_index[p_idx];
        }

        if (!parent_expanded) continue;

        if (current_y + item_height > self->screen_bounds.y + self->screen_bounds.height) break; // Clip

        int32_t nx = self->screen_bounds.x + 4 + depth * indent_step;
        
        // Draw +/- Expand toggle indicator box (8x8)
        BWE_DrawRect(fb, nx, current_y + 6, 8, 8, 0xFF475569, 1);
        if (self->control_data.treeview.expanded[i]) {
            BWE_DrawText(fb, "-", nx + 2, current_y + 2, 0xFF000000, 0);
        } else {
            BWE_DrawText(fb, "+", nx + 1, current_y + 3, 0xFF000000, 0);
        }

        // Draw node item text
        int32_t tx = nx + 14;
        if ((int32_t)i == selected) {
            BWE_FillRect(fb, tx - 2, current_y, self->screen_bounds.width - (tx - self->screen_bounds.x) - 4, item_height, BWE_ThemeGetColor(BWE_THEME_SELECTION_BG));
            BWE_DrawText(fb, self->control_data.treeview.nodes[i], tx, current_y + 4, BWE_ThemeGetColor(BWE_THEME_SELECTION_TEXT), 0);
        } else {
            BWE_DrawText(fb, self->control_data.treeview.nodes[i], tx, current_y + 4, BWE_ThemeGetColor(BWE_THEME_TEXT), 0);
        }

        current_y += item_height;
    }
}

static void bwe_treeview_event(uint32_t window_id, const BWE_Event* event) {
    BWE_Window* self = BWE_GetWindow(window_id);
    if (!self) return;

    int32_t item_height = 20;
    int32_t indent_step = 16;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t my = event->data.mouse.y - self->screen_bounds.y;
        int32_t mx = event->data.mouse.x - self->screen_bounds.x;

        // Traverse layout sequence to map clicked line
        int32_t current_y = 2;
        uint32_t clicked_node_idx = 99;

        for (uint32_t i = 0; i < self->control_data.treeview.node_count; i++) {
            // Check expansion
            bool parent_expanded = true;
            int32_t p_idx = self->control_data.treeview.parent_node_index[i];
            while (p_idx != -1) {
                if (!self->control_data.treeview.expanded[p_idx]) {
                    parent_expanded = false;
                    break;
                }
                p_idx = self->control_data.treeview.parent_node_index[p_idx];
            }
            if (!parent_expanded) continue;

            if (my >= current_y && my < current_y + item_height) {
                clicked_node_idx = i;
                break;
            }
            current_y += item_height;
        }

        if (clicked_node_idx < self->control_data.treeview.node_count) {
            int32_t depth = 0;
            int32_t parent_idx = self->control_data.treeview.parent_node_index[clicked_node_idx];
            while (parent_idx != -1) {
                depth++;
                parent_idx = self->control_data.treeview.parent_node_index[parent_idx];
            }
            int32_t nx = 4 + depth * indent_step;

            // Check if +/- indicator box clicked
            if (mx >= nx && mx <= nx + 12) {
                self->control_data.treeview.expanded[clicked_node_idx] = !self->control_data.treeview.expanded[clicked_node_idx];
            } else {
                self->control_data.treeview.selected_index = (int32_t)clicked_node_idx;
                if (self->control_data.treeview.on_node_select) {
                    self->control_data.treeview.on_node_select(window_id, clicked_node_idx);
                }
            }
            BWE_InvalidateWindow(window_id);
        }
    }
}

bwe_error_t BOS_CreateTreeView(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t* out_id) {
    uint32_t id = 0;
    bwe_error_t err = BOS_CreateSurface(parent_id, x, y, width, height, BWE_WINDOW_CHILD | BWE_WINDOW_BORDERLESS, &id);
    if (err != BWE_SUCCESS) return err;

    BWE_Window* win = BWE_GetWindow(id);
    if (win) {
        win->type = BWE_TYPE_TREEVIEW;
        win->control_data.treeview.node_count = 0;
        win->control_data.treeview.selected_index = -1;
        win->control_data.treeview.on_node_select = 0;

        win->on_render = bwe_treeview_render;
        win->on_event = bwe_treeview_event;
    }

    if (out_id) *out_id = id;
    return BWE_SUCCESS;
}

bwe_error_t BOS_TreeView_AddNode(uint32_t tree_id, const char* name, int32_t parent_node_idx, int32_t* out_node_idx) {
    BWE_Window* win = BWE_GetWindow(tree_id);
    if (!win || win->type != BWE_TYPE_TREEVIEW) return BWE0001;

    uint32_t idx = win->control_data.treeview.node_count;
    if (idx >= 16) return BWE0004;

    if (name) {
        uint32_t i = 0;
        for (; name[i] != '\0' && i < 63; i++) {
            win->control_data.treeview.nodes[idx][i] = name[i];
        }
        win->control_data.treeview.nodes[idx][i] = '\0';
    } else {
        win->control_data.treeview.nodes[idx][0] = '\0';
    }

    win->control_data.treeview.parent_node_index[idx] = parent_node_idx;
    win->control_data.treeview.expanded[idx] = true; // Default expanded

    win->control_data.treeview.node_count++;
    
    if (out_node_idx) *out_node_idx = (int32_t)idx;
    BWE_InvalidateWindow(tree_id);
    return BWE_SUCCESS;
}
