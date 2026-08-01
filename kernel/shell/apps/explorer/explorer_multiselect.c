#include "explorer_multiselect.h"

void explorer_multiselect_box_start(ExplorerSelectionState* sel, int32_t x, int32_t y) {
    if (!sel) return;
    sel->is_box_selecting = true;
    sel->box_start_x = x;
    sel->box_start_y = y;
    sel->box_end_x = x;
    sel->box_end_y = y;
}

void explorer_multiselect_box_update(ExplorerSelectionState* sel, int32_t x, int32_t y) {
    if (!sel || !sel->is_box_selecting) return;
    sel->box_end_x = x;
    sel->box_end_y = y;
}

void explorer_multiselect_box_finish(ExplorerSelectionState* sel) {
    if (!sel) return;
    sel->is_box_selecting = false;
}
