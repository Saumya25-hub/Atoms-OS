#include "../include/shell32_api.h"

BOOL shell_desktop_init(void) {
    return true;
}

BOOL shell_desktop_set_wallpaper(const char* path) {
    (void)path;
    return true;
}
