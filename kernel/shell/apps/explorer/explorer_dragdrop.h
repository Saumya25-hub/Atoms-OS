#ifndef BSEC_EXPLORER_DRAGDROP_H
#define BSEC_EXPLORER_DRAGDROP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool    is_dragging;
    int32_t drag_item_idx;
    char    drag_path[256];
    int32_t start_x;
    int32_t start_y;
    int32_t current_x;
    int32_t current_y;
    int32_t target_folder_idx;
} ExplorerDragDropState;

void explorer_dragdrop_init(ExplorerDragDropState* dd);
void explorer_dragdrop_start(ExplorerDragDropState* dd, int32_t item_idx, const char* path, int32_t x, int32_t y);
void explorer_dragdrop_update(ExplorerDragDropState* dd, int32_t x, int32_t y, int32_t target_idx);
bool explorer_dragdrop_finish(ExplorerDragDropState* dd, char* out_src_path, int32_t* out_target_idx);
void explorer_dragdrop_cancel(ExplorerDragDropState* dd);

#endif // BSEC_EXPLORER_DRAGDROP_H
