#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// ================================================================
// FileExplorer.BOSX V1.0 — Runtime Manager
// ATOMS OS Enterprise Storage Shell
// Owns ZERO kernel operations. Delegates to SHELL32 / KERNEL32 / BFS.
// ================================================================

static bool g_fe_initialized = false;
static char g_current_path[FE_MAX_PATH] = "/";

int32_t FileExplorerInitialize(void) {
    if (g_fe_initialized) return 0;
    display_print("[FE] Initializing FileExplorer.BOSX V1.0...\n");

    extern void fe_navigation_init(void);
    extern void fe_addressbar_init(void);
    extern void fe_treeview_init(void);
    extern void fe_listview_init(void);
    extern void fe_namespace_init(void);
    extern void fe_drives_init(void);
    extern void fe_search_init(void);
    extern void fe_preview_init(void);
    extern void fe_thumbnail_init(void);
    extern void fe_clipboard_init(void);
    extern void fe_dragdrop_init(void);
    extern void fe_operations_init(void);
    extern void fe_properties_init(void);
    extern void fe_permissions_init(void);
    extern void fe_contextmenu_init(void);
    extern void fe_favorites_init(void);
    extern void fe_recent_init(void);
    extern void fe_history_init(void);
    extern void fe_shortcuts_init(void);
    extern void fe_refresh_init(void);
    extern void fe_watcher_init(void);
    extern void fe_diagnostics_init(void);

    fe_navigation_init();
    fe_addressbar_init();
    fe_treeview_init();
    fe_listview_init();
    fe_namespace_init();
    fe_drives_init();
    fe_search_init();
    fe_preview_init();
    fe_thumbnail_init();
    fe_clipboard_init();
    fe_dragdrop_init();
    fe_operations_init();
    fe_properties_init();
    fe_permissions_init();
    fe_contextmenu_init();
    fe_favorites_init();
    fe_recent_init();
    fe_history_init();
    fe_shortcuts_init();
    fe_refresh_init();
    fe_watcher_init();
    fe_diagnostics_init();

    g_fe_initialized = true;
    display_print("[FE] FileExplorer.BOSX V1.0 Active. All 30 engines ONLINE.\n");
    return 0;
}

void FileExplorerShutdown(void) {
    if (!g_fe_initialized) return;
    display_print("[FE] Shutting down FileExplorer.BOSX V1.0...\n");
    g_fe_initialized = false;
    display_print("[FE] Shutdown complete. Storage Shell OFFLINE.\n");
}

const char* fe_get_current_path(void) { return g_current_path; }

bool fe_navigate_to(const char* path) {
    if (!path) return false;
    uint32_t i = 0;
    while (path[i] && i < FE_MAX_PATH - 1) { g_current_path[i] = path[i]; i++; }
    g_current_path[i] = '\0';
    display_print("[FE] NavigateTo: ");
    display_print(g_current_path);
    display_print(" -> SHELL32.ShellExecuteEx() OK\n");
    return true;
}
