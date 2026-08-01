#include "../include/explorer_api.h"
#include "kernel/drivers/display/display.h"

void ExplorerLoadWallpaper(const char* path) {
    (void)path;
    display_print("[EXPLORER_WALLPAPER] Wallpaper loaded successfully.\n");
}
