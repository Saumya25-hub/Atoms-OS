#include "explorer_selection.h"
#include "kernel/core/lib/include/string.h"

void explorer_selection_init(ExplorerSelectionState* sel) {
    if (!sel) return;
    memset(sel, 0, sizeof(ExplorerSelectionState));
    sel->primary_selected = -1;
    sel->last_clicked = -1;
}

void explorer_selection_clear(ExplorerSelectionState* sel) {
    if (!sel) return;
    memset(sel->selected_mask, 0, sizeof(sel->selected_mask));
    sel->primary_selected = -1;
    sel->selected_count = 0;
}

void explorer_selection_single(ExplorerSelectionState* sel, int32_t index, uint32_t total_items) {
    if (!sel) return;
    explorer_selection_clear(sel);
    if (index >= 0 && (uint32_t)index < total_items && (uint32_t)index < BSEC_MAX_SELECTION_ITEMS) {
        sel->selected_mask[index] = true;
        sel->primary_selected = index;
        sel->last_clicked = index;
        sel->selected_count = 1;
    }
}

void explorer_selection_toggle(ExplorerSelectionState* sel, int32_t index, uint32_t total_items) {
    if (!sel || index < 0 || (uint32_t)index >= total_items || (uint32_t)index >= BSEC_MAX_SELECTION_ITEMS) return;
    
    sel->selected_mask[index] = !sel->selected_mask[index];
    if (sel->selected_mask[index]) {
        sel->primary_selected = index;
        sel->selected_count++;
    } else {
        if (sel->selected_count > 0) sel->selected_count--;
        if (sel->primary_selected == index) sel->primary_selected = -1;
    }
    sel->last_clicked = index;
}

void explorer_selection_range(ExplorerSelectionState* sel, int32_t from_idx, int32_t to_idx, uint32_t total_items) {
    if (!sel) return;
    explorer_selection_clear(sel);
    if (from_idx < 0) from_idx = 0;
    if (to_idx < 0) to_idx = 0;
    if ((uint32_t)from_idx >= total_items) from_idx = total_items - 1;
    if ((uint32_t)to_idx >= total_items) to_idx = total_items - 1;
    
    int32_t start = (from_idx < to_idx) ? from_idx : to_idx;
    int32_t end   = (from_idx < to_idx) ? to_idx : from_idx;
    
    for (int32_t i = start; i <= end && (uint32_t)i < BSEC_MAX_SELECTION_ITEMS; i++) {
        sel->selected_mask[i] = true;
        sel->selected_count++;
    }
    sel->primary_selected = to_idx;
    sel->last_clicked = to_idx;
}

void explorer_selection_all(ExplorerSelectionState* sel, uint32_t total_items) {
    if (!sel) return;
    explorer_selection_clear(sel);
    uint32_t limit = (total_items < BSEC_MAX_SELECTION_ITEMS) ? total_items : BSEC_MAX_SELECTION_ITEMS;
    for (uint32_t i = 0; i < limit; i++) {
        sel->selected_mask[i] = true;
    }
    sel->selected_count = limit;
    if (limit > 0) sel->primary_selected = 0;
}

void explorer_selection_invert(ExplorerSelectionState* sel, uint32_t total_items) {
    if (!sel) return;
    uint32_t limit = (total_items < BSEC_MAX_SELECTION_ITEMS) ? total_items : BSEC_MAX_SELECTION_ITEMS;
    sel->selected_count = 0;
    sel->primary_selected = -1;
    for (uint32_t i = 0; i < limit; i++) {
        sel->selected_mask[i] = !sel->selected_mask[i];
        if (sel->selected_mask[i]) {
            sel->selected_count++;
            if (sel->primary_selected < 0) sel->primary_selected = (int32_t)i;
        }
    }
}

bool explorer_selection_is_selected(const ExplorerSelectionState* sel, int32_t index) {
    if (!sel || index < 0 || (uint32_t)index >= BSEC_MAX_SELECTION_ITEMS) return false;
    return sel->selected_mask[index];
}
