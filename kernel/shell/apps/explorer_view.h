#ifndef BOS_EXPLORER_VIEW_H
#define BOS_EXPLORER_VIEW_H

#include "explorer.h"
#include "kernel/wm/bwe/include/bwe.h"

// Public API
void explorer_view_paint(uint32_t canvas_id, const BVFramebuffer* fb, const BWE_Rect* clip);
void explorer_view_render(ExplorerContext* ctx);
int32_t explorer_view_hit_test(ExplorerContext* ctx, int32_t local_x, int32_t local_y);
void explorer_view_handle_click(ExplorerContext* ctx, int32_t local_x, int32_t local_y, bool double_click);

#endif // BOS_EXPLORER_VIEW_H
