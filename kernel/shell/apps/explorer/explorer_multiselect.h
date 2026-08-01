#ifndef BSEC_EXPLORER_MULTISELECT_H
#define BSEC_EXPLORER_MULTISELECT_H

#include "explorer_selection.h"

// Multi-select helper utilities
void explorer_multiselect_box_start(ExplorerSelectionState* sel, int32_t x, int32_t y);
void explorer_multiselect_box_update(ExplorerSelectionState* sel, int32_t x, int32_t y);
void explorer_multiselect_box_finish(ExplorerSelectionState* sel);

#endif // BSEC_EXPLORER_MULTISELECT_H
