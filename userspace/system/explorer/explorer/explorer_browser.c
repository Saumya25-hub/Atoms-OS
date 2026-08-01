#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerOpenFolder(const char* path) {
    (void)path;
    display_print("[EXPLORER_BROWSER] Opening File Explorer Window...\n");
}
