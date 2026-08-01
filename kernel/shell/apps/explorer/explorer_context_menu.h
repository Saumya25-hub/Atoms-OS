#ifndef BSEC_EXPLORER_CONTEXT_MENU_H
#define BSEC_EXPLORER_CONTEXT_MENU_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CONTEXT_MENU_TARGET_BG = 0,
    CONTEXT_MENU_TARGET_FILE,
    CONTEXT_MENU_TARGET_FOLDER
} ContextMenuTargetType;

typedef enum {
    MENU_CMD_NONE = 0,
    MENU_CMD_OPEN,
    MENU_CMD_COPY,
    MENU_CMD_CUT,
    MENU_CMD_PASTE,
    MENU_CMD_DELETE,
    MENU_CMD_RENAME,
    MENU_CMD_NEW_FOLDER,
    MENU_CMD_REFRESH,
    MENU_CMD_PROPERTIES
} ContextMenuCommand;

typedef struct {
    bool                  is_visible;
    int32_t               x;
    int32_t               y;
    int32_t               w;
    int32_t               h;
    ContextMenuTargetType target_type;
    char                  target_name[128];
    int32_t               hovered_item_idx;
} ExplorerContextMenu;

void explorer_ctxmenu_init(ExplorerContextMenu* menu);
void explorer_ctxmenu_show(ExplorerContextMenu* menu, int32_t x, int32_t y, ContextMenuTargetType target, const char* name);
void explorer_ctxmenu_hide(ExplorerContextMenu* menu);
ContextMenuCommand explorer_ctxmenu_hit_test(const ExplorerContextMenu* menu, int32_t click_x, int32_t click_y);

#endif // BSEC_EXPLORER_CONTEXT_MENU_H
