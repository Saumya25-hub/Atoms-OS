#ifndef BOS_EXPLORER_H
#define BOS_EXPLORER_H

#include <stdint.h>
#include <stdbool.h>
#include "explorer_cache.h"
#include "explorer_profiler.h"

// View mode types
typedef enum {
    EXP_VIEW_ICON = 0,
    EXP_VIEW_LIST,
    EXP_VIEW_DETAILS
} ExplorerViewMode;

// Main Context for an Explorer Instance
typedef struct {
    uint32_t window_id;
    
    // Core Layout Containers
    uint32_t toolbar_id;
    uint32_t sidebar_id;
    uint32_t view_panel_id;
    uint32_t canvas_id;       // Single canvas rendering surface
    uint32_t scrollbar_id;    // Viewport vertical scrollbar
    uint32_t statusbar_id;
    
    // Toolbar Elements
    uint32_t pathbar_id;
    
    // Status Elements
    uint32_t status_label_id;
    
    // State & Cache
    char             current_path[256];
    ExplorerViewMode view_mode;
    int32_t          scroll_offset_y;
    int32_t          selected_index;
    int32_t          hovered_index;
    
    ExplorerDirCache dir_cache;
    ExplorerProfiler profiler;
} ExplorerContext;

// Public entry point registered with HORSE / Shell
int explorer_init(uint32_t* out_win);

// Core Navigation API
void explorer_navigate(ExplorerContext* ctx, const char* path);

// View Mode API
void explorer_set_view_mode(ExplorerContext* ctx, ExplorerViewMode mode);

#endif // BOS_EXPLORER_H
