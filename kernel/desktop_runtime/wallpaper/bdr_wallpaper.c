#include "../include/bdr_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BDR_SetWallpaper(BDrSession* session, const char* image_path, BDrWallpaperMode mode) {
    if (!session || !session->active) return -1;

    if (image_path) {
        strcpy(session->wallpaper_path, image_path);
    } else {
        session->wallpaper_path[0] = '\0';
    }
    session->wallpaper_mode = mode;
    return 0;
}
