#include "../include/shell32_api.h"

BOOL ShellShowContextMenu(HANDLE hwndOwner, LPCSTR lpPath, int x, int y) {
    (void)hwndOwner; (void)lpPath; (void)x; (void)y;
    return true;
}
