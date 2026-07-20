#include "../include/bwe_layout.h"
#include "../include/bwe_geometry.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

void BWE_UpdateLayout(uint32_t parent_id) {
    BWE_Window* parent = BWE_GetWindow(parent_id);
    if (!parent) return;

    BWE_Rect p_client;
    if (parent_id == BWE_DESKTOP_ID) {
        p_client = parent->screen_bounds;
    } else {
        BWE_Geometry_CalculateClientBounds(parent, &p_client);
    }
    
    int32_t new_pw = p_client.width;
    int32_t new_ph = p_client.height;

    for (uint32_t i = 0; i < parent->child_count; i++) {
        BWE_Window* child = BWE_GetWindow(parent->children[i]);
        if (!child || child->state == BWE_STATE_HIDDEN) continue;

        if (child->anchor_flags == BWE_ANCHOR_NONE || child->baseline_parent_w == 0 || child->baseline_parent_h == 0) {
            // Unanchored controls simply track their exact original local position
            // screen_bounds just needs to shift with parent_client.
            child->screen_bounds.x = p_client.x + child->local_bounds.x;
            child->screen_bounds.y = p_client.y + child->local_bounds.y;
            child->screen_bounds.width = child->local_bounds.width;
            child->screen_bounds.height = child->local_bounds.height;
            BWE_UpdateLayout(child->id);
            continue;
        }

        int32_t nx = child->local_bounds.x;
        int32_t ny = child->local_bounds.y;
        int32_t nw = child->local_bounds.width;
        int32_t nh = child->local_bounds.height;

        // HORIZONTAL ANCHOR MATH
        if ((child->anchor_flags & BWE_ANCHOR_LEFT) && (child->anchor_flags & BWE_ANCHOR_RIGHT)) {
            // Stretches horizontally
            nw = new_pw - child->margins.left - child->margins.right;
            nx = child->margins.left;
        } else if (child->anchor_flags & BWE_ANCHOR_RIGHT) {
            // Sticks to right
            nx = new_pw - child->margins.right - nw;
        } else if (child->anchor_flags & BWE_ANCHOR_LEFT) {
            // Sticks to left
            nx = child->margins.left;
        } else {
            // No horizontal anchors: floats proportionally or remains relative to left?
            // Usually no anchors = stays at fixed distance from left, or floats center. 
            // We'll keep it at fixed left distance for simplicity.
        }

        // VERTICAL ANCHOR MATH
        if ((child->anchor_flags & BWE_ANCHOR_TOP) && (child->anchor_flags & BWE_ANCHOR_BOTTOM)) {
            // Stretches vertically
            nh = new_ph - child->margins.top - child->margins.bottom;
            ny = child->margins.top;
        } else if (child->anchor_flags & BWE_ANCHOR_BOTTOM) {
            // Sticks to bottom
            ny = new_ph - child->margins.bottom - nh;
        } else if (child->anchor_flags & BWE_ANCHOR_TOP) {
            // Sticks to top
            ny = child->margins.top;
        }

        if (nw < 0) nw = 0;
        if (nh < 0) nh = 0;

        // Directly update bounds without calling BOS_SetBounds (which resets margins)
        child->local_bounds.x = nx;
        child->local_bounds.y = ny;
        child->local_bounds.width = nw;
        child->local_bounds.height = nh;
        
        child->screen_bounds.x = p_client.x + nx;
        child->screen_bounds.y = p_client.y + ny;
        child->screen_bounds.width = nw;
        child->screen_bounds.height = nh;

        // Recurse Layout
        BWE_UpdateLayout(child->id);
    }
}

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

    if (win->parent_id == BWE_DESKTOP_ID) {
        win->restore_bounds = win->screen_bounds;
        win->flags |= BWE_WINDOW_FULLSCREEN;
        // The desktop size is g_kernel_screen_width/height, but ATOMS OS has a taskbar.
        // Assuming taskbar is BWE_TYPE_TASKBAR, but we'll just maximize to full screen for now.
        // In the future, subtract taskbar height.
        extern uint32_t g_kernel_screen_width;
        extern uint32_t g_kernel_screen_height;
        BOS_SetBounds(window_id, 0, 0, g_kernel_screen_width, g_kernel_screen_height - 32); // Assuming taskbar is 32px bottom
    }
}

void BWE_WindowRestore(uint32_t window_id) {
    BWE_Window* win = BWE_GetWindow(window_id);
    if (!win || !BWE_WindowIsMaximized(window_id)) return;

    win->flags &= ~BWE_WINDOW_FULLSCREEN;
    BOS_SetBounds(window_id, win->restore_bounds.x, win->restore_bounds.y, win->restore_bounds.width, win->restore_bounds.height);
}
