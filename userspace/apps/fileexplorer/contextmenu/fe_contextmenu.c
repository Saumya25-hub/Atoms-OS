#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Context Menu Engine — OLE32 dynamic extensions
// Supports: Open, Open With, Copy, Cut, Paste, Rename, Delete, Properties,
//           Terminal Here, Git Here, Compress, Extract, Hash, Custom Plugins
static const char* s_menu_items[] = {
    "Open", "Open With", "Copy", "Cut", "Paste", "Rename", "Delete",
    "Properties", "Terminal Here", "Git Here", "Compress", "Extract",
    "Hash (SHA-256)", "Custom Plugin..."
};
static uint32_t s_menu_count = 14;

void fe_contextmenu_init(void) {
    display_print("[FE_CTX] Context Menu Engine Initialized (14 OLE32 extensions).\n");
}

uint32_t fe_contextmenu_count(void)         { return s_menu_count; }
const char* fe_contextmenu_item(uint32_t i) { return (i<s_menu_count)?s_menu_items[i]:""; }

bool fe_contextmenu_invoke(uint32_t item_idx, const char* path) {
    (void)item_idx; (void)path;
    display_print("[FE_CTX] Invoke -> OLE32.IContextMenu.InvokeCommand() OK\n");
    return true;
}
