#include "explorer_dragdrop.h"
#include "kernel/core/lib/include/string.h"

void explorer_dragdrop_init(ExplorerDragDropState* dd) {
    if (!dd) return;
    memset(dd, 0, sizeof(ExplorerDragDropState));
    dd->drag_item_idx = -1;
    dd->target_folder_idx = -1;
}

void explorer_dragdrop_start(ExplorerDragDropState* dd, int32_t item_idx, const char* path, int32_t x, int32_t y) {
    if (!dd || !path) return;
    dd->is_dragging = true;
    dd->drag_item_idx = item_idx;
    strcpy(dd->drag_path, path);
    dd->start_x = x;
    dd->start_y = y;
    dd->current_x = x;
    dd->current_y = y;
    dd->target_folder_idx = -1;
}

void explorer_dragdrop_update(ExplorerDragDropState* dd, int32_t x, int32_t y, int32_t target_idx) {
    if (!dd || !dd->is_dragging) return;
    dd->current_x = x;
    dd->current_y = y;
    dd->target_folder_idx = target_idx;
}

bool explorer_dragdrop_finish(ExplorerDragDropState* dd, char* out_src_path, int32_t* out_target_idx) {
    if (!dd || !dd->is_dragging) return false;
    if (out_src_path) strcpy(out_src_path, dd->drag_path);
    if (out_target_idx) *out_target_idx = dd->target_folder_idx;
    bool moved = (dd->target_folder_idx >= 0 && dd->target_folder_idx != dd->drag_item_idx);
    explorer_dragdrop_cancel(dd);
    return moved;
}

void explorer_dragdrop_cancel(ExplorerDragDropState* dd) {
    if (!dd) return;
    memset(dd, 0, sizeof(ExplorerDragDropState));
    dd->drag_item_idx = -1;
    dd->target_folder_idx = -1;
}
