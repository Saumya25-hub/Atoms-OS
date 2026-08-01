#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerOpenDesktop(void) {
    display_print("[EXPLORER_DESKTOP] Navigating to Desktop Virtual Folder...\n");
}

void ExplorerRefreshDesktop(void) {
    display_print("[EXPLORER_DESKTOP] Refreshing Desktop Workspace, Icons & Wallpaper...\n");
}

void ExplorerLockDesktop(void) {
    display_print("[EXPLORER_DESKTOP] Desktop Session Locked.\n");
}
