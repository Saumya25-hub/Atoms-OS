#ifndef BOS_EXPLORER_H
#define BOS_EXPLORER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/bsom/include/bsom_api.h"
#include "kernel/wm/bwe/include/bwe.h"

// ============================================================
// ATOMS OS Explorer — Pure View Layer (Phase 9 Rewrite)
// ============================================================
// Explorer owns ONLY view state. Zero filesystem ownership.
// ALL operations delegate to BSOM universal object layer.
// ============================================================

#define EXPLORER_MAX_VIEW_ITEMS   4096
#define EXPLORER_HISTORY_MAX      64

// ExplorerViewItem: Immutable view object received from BSOM.
// Explorer NEVER creates these from filesystem data.
typedef struct {
    BSOMObject*  obj;           // BSOM object handle (owned by BSOM)
    uint32_t     icon_id;      // Resolved by BSOM_GetIcon()
    bool         is_selected;  // View-only selection state
} ExplorerViewItem;

// ExplorerContext: Pure view state. No filesystem data.
typedef struct {
    // Window
    uint32_t      window_id;

    // Current BSOM folder object
    BSOMObject*   current_folder;

    // View items (populated from BSOM_GetChildren)
    ExplorerViewItem view_items[EXPLORER_MAX_VIEW_ITEMS];
    uint32_t      view_item_count;

    // Navigation History (path strings for BSOM_OpenObject)
    char          history[EXPLORER_HISTORY_MAX][BDE_PATH_MAX];
    int32_t       history_pos;
    int32_t       history_count;

    // View state only
    int32_t       scroll_y;
    int32_t       selected_index;

    // Context Menu State
    bool          ctx_menu_open;
    int32_t       ctx_menu_x;
    int32_t       ctx_menu_y;
    bool          ctx_menu_is_item;
} ExplorerContext;

// Public API — Pure View Layer
int   Explorer_Create(uint32_t* out_win);
void  Explorer_Destroy(uint32_t win_id);
void  Explorer_Navigate(ExplorerContext* ctx, const char* path);
void  Explorer_Refresh(ExplorerContext* ctx);
void  Explorer_Back(ExplorerContext* ctx);
void  Explorer_Forward(ExplorerContext* ctx);
void  Explorer_Up(ExplorerContext* ctx);

// Legacy compat wrapper
int  explorer_init(uint32_t* out_win);
void explorer_navigate(ExplorerContext* ctx, const char* path);

#endif // BOS_EXPLORER_H
