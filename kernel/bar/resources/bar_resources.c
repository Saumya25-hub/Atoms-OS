#include "../include/bar_api.h"

BARResourceID BAR_LoadResource(BARProcessID pid, const char* res_path) {
    (void)pid;
    if (!res_path) return 0;
    return (BARResourceID)0x2001;
}

BARResourceID BAR_LoadIcon(const char* icon_name) {
    if (!icon_name) return 0;
    return (BARResourceID)0x3001;
}

BARResourceID BAR_LoadCursor(const char* cursor_name) {
    if (!cursor_name) return 0;
    return (BARResourceID)0x4001;
}
