#ifndef BWE_LAYOUT_H
#define BWE_LAYOUT_H

#include "bwe.h"

// Dispatches a layout update on a parent container, recursively adjusting children
void BWE_UpdateLayout(uint32_t parent_id);

// Sets the anchor mode of a control, automatically requesting a layout update
void BWE_SetAnchorMode(uint32_t window_id, uint8_t anchor_flags);

// Maximize / Restore Operations
void BWE_WindowMaximize(uint32_t window_id);
void BWE_WindowRestore(uint32_t window_id);
bool BWE_WindowIsMaximized(uint32_t window_id);

#endif // BWE_LAYOUT_H
