#include "../include/bwe.h"

// External registry lookup
extern BWE_Window* BWE_GetWindow(uint32_t window_id);

void BWE_UpdateLayout(uint32_t parent_id) {
    BWE_Window* parent = BWE_GetWindow(parent_id);
    if (!parent || parent->child_count == 0) return;

    // Start with parent screen bounds
    BWE_Rect area = parent->screen_bounds;

    // If parent is a window (not borderless), adjust for the titlebar and borders
    if (parent->type == BWE_TYPE_WINDOW && !(parent->flags & BWE_WINDOW_BORDERLESS)) {
        area.x += 5;
        area.y += 35;
        area.width -= 10;
        area.height -= 40;
    }

    // Apply parent inner padding bounds
    area.x += parent->padding.left;
    area.y += parent->padding.top;
    area.width -= (parent->padding.left + parent->padding.right);
    area.height -= (parent->padding.top + parent->padding.bottom);

    // Track remaining layable boundaries
    int32_t left = area.x;
    int32_t top = area.y;
    int32_t right = area.x + area.width;
    int32_t bottom = area.y + area.height;

    // Loop through children to set layout alignments
    for (uint32_t i = 0; i < parent->child_count; i++) {
        BWE_Window* child = BWE_GetWindow(parent->children[i]);
        if (!child || child->state == BWE_STATE_HIDDEN) continue;

        child->old_screen_bounds = child->screen_bounds;

        int32_t cx = child->local_bounds.x;
        int32_t cy = child->local_bounds.y;
        int32_t cw = child->local_bounds.width;
        int32_t ch = child->local_bounds.height;

        int32_t ml = child->margins.left;
        int32_t mt = child->margins.top;
        int32_t mr = child->margins.right;
        int32_t mb = child->margins.bottom;

        switch (child->dock_mode) {
            case BWE_DOCK_TOP:
                cx = left + ml;
                cy = top + mt;
                cw = (right - left) - (ml + mr);
                ch = child->local_bounds.height;
                top += ch + mt + mb;
                break;
            case BWE_DOCK_BOTTOM:
                cx = left + ml;
                cy = (bottom - ch) - mb;
                cw = (right - left) - (ml + mr);
                ch = child->local_bounds.height;
                bottom -= ch + mt + mb;
                break;
            case BWE_DOCK_LEFT:
                cx = left + ml;
                cy = top + mt;
                cw = child->local_bounds.width;
                ch = (bottom - top) - (mt + mb);
                left += cw + ml + mr;
                break;
            case BWE_DOCK_RIGHT:
                cx = (right - cw) - mr;
                cy = top + mt;
                cw = child->local_bounds.width;
                ch = (bottom - top) - (mt + mb);
                right -= cw + ml + mr;
                break;
            case BWE_DOCK_FILL:
                cx = left + ml;
                cy = top + mt;
                cw = (right - left) - (ml + mr);
                ch = (bottom - top) - (mt + mb);
                break;
            case BWE_DOCK_CENTER:
                cx = left + ((right - left) - cw) / 2;
                cy = top + ((bottom - top) - ch) / 2;
                break;
            case BWE_DOCK_NONE:
            default:
                // Absolute positioning: compute bounds relative to client area
                cx = parent->screen_bounds.x + child->local_bounds.x;
                cy = parent->screen_bounds.y + child->local_bounds.y;
                if (parent->type == BWE_TYPE_WINDOW && !(parent->flags & BWE_WINDOW_BORDERLESS)) {
                    cx += 5;
                    cy += 35;
                }
                child->screen_bounds.x = cx;
                child->screen_bounds.y = cy;
                child->screen_bounds.width = cw;
                child->screen_bounds.height = ch;
                BWE_UpdateLayout(child->id);
                continue;
        }

        child->screen_bounds.x = cx;
        child->screen_bounds.y = cy;
        child->screen_bounds.width = (cw > 0) ? cw : 0;
        child->screen_bounds.height = (ch > 0) ? ch : 0;

        BWE_UpdateLayout(child->id);
    }
}
