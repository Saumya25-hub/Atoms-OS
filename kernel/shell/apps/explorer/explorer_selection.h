#ifndef BSEC_EXPLORER_SELECTION_H
#define BSEC_EXPLORER_SELECTION_H

#include <stdint.h>
#include <stdbool.h>

#define BSEC_MAX_SELECTION_ITEMS 10000

typedef struct {
    bool     selected_mask[BSEC_MAX_SELECTION_ITEMS];
    int32_t  primary_selected;
    int32_t  last_clicked;
    uint32_t selected_count;
    
    // Drag selection rect
    bool    is_box_selecting;
    int32_t box_start_x;
    int32_t box_start_y;
    int32_t box_end_x;
    int32_t box_end_y;
} ExplorerSelectionState;

void explorer_selection_init(ExplorerSelectionState* sel);
void explorer_selection_clear(ExplorerSelectionState* sel);
void explorer_selection_single(ExplorerSelectionState* sel, int32_t index, uint32_t total_items);
void explorer_selection_toggle(ExplorerSelectionState* sel, int32_t index, uint32_t total_items);
void explorer_selection_range(ExplorerSelectionState* sel, int32_t from_idx, int32_t to_idx, uint32_t total_items);
void explorer_selection_all(ExplorerSelectionState* sel, uint32_t total_items);
void explorer_selection_invert(ExplorerSelectionState* sel, uint32_t total_items);
bool explorer_selection_is_selected(const ExplorerSelectionState* sel, int32_t index);

#endif // BSEC_EXPLORER_SELECTION_H
