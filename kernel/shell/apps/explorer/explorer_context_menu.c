#include "explorer_context_menu.h"
#include "kernel/core/lib/include/string.h"

void explorer_ctxmenu_init(ExplorerContextMenu* menu) {
    if (!menu) return;
    memset(menu, 0, sizeof(ExplorerContextMenu));
    menu->hovered_item_idx = -1;
}

void explorer_ctxmenu_show(ExplorerContextMenu* menu, int32_t x, int32_t y, ContextMenuTargetType target, const char* name) {
    if (!menu) return;
    menu->is_visible = true;
    menu->x = x;
    menu->y = y;
    menu->w = 140;
    menu->target_type = target;
    menu->hovered_item_idx = -1;
    if (name) strcpy(menu->target_name, name);
    else menu->target_name[0] = '\0';
    
    if (target == CONTEXT_MENU_TARGET_BG) {
        menu->h = 4 * 24 + 10; // New Folder, Paste, Refresh, Properties
    } else {
        menu->h = 6 * 24 + 10; // Open, Copy, Cut, Rename, Delete, Properties
    }
}

void explorer_ctxmenu_hide(ExplorerContextMenu* menu) {
    if (!menu) return;
    menu->is_visible = false;
}

ContextMenuCommand explorer_ctxmenu_hit_test(const ExplorerContextMenu* menu, int32_t click_x, int32_t click_y) {
    if (!menu || !menu->is_visible) return MENU_CMD_NONE;
    
    if (click_x < menu->x || click_x > menu->x + menu->w || click_y < menu->y || click_y > menu->y + menu->h) {
        return MENU_CMD_NONE;
    }
    
    int32_t rel_y = click_y - (menu->y + 5);
    int32_t item_idx = rel_y / 24;
    
    if (menu->target_type == CONTEXT_MENU_TARGET_BG) {
        switch (item_idx) {
            case 0: return MENU_CMD_NEW_FOLDER;
            case 1: return MENU_CMD_PASTE;
            case 2: return MENU_CMD_REFRESH;
            case 3: return MENU_CMD_PROPERTIES;
            default: return MENU_CMD_NONE;
        }
    } else {
        switch (item_idx) {
            case 0: return MENU_CMD_OPEN;
            case 1: return MENU_CMD_COPY;
            case 2: return MENU_CMD_CUT;
            case 3: return MENU_CMD_RENAME;
            case 4: return MENU_CMD_DELETE;
            case 5: return MENU_CMD_PROPERTIES;
            default: return MENU_CMD_NONE;
        }
    }
}
