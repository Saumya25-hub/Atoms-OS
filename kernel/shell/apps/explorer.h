#ifndef BOS_EXPLORER_H
#define BOS_EXPLORER_H

#include <stdint.h>
#include <stdbool.h>

// View mode types
typedef enum {
    EXP_VIEW_ICON,
    EXP_VIEW_LIST,
    EXP_VIEW_DETAILS
} ExplorerViewMode;

// Main context for an Explorer instance
typedef struct {
    uint32_t window_id;
    
    // Core Layout Panels
    uint32_t toolbar_id;
    uint32_t sidebar_id;
    uint32_t view_panel_id;
    uint32_t statusbar_id;
    
    // Toolbar Elements
    uint32_t pathbar_id;
    
    // Status Elements
    uint32_t status_label_id;
    
    // State
    char current_path[256];
    ExplorerViewMode view_mode;
    
} ExplorerContext;

// Public entry point registered with HORSE
int explorer_init(uint32_t* out_win);

// Core Navigation API
void explorer_navigate(ExplorerContext* ctx, const char* path);

// View Mode API
void explorer_set_view_mode(ExplorerContext* ctx, ExplorerViewMode mode);

#endif // BOS_EXPLORER_H
