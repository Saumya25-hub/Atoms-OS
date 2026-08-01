#include "../include/shell32_api.h"

static uint32_t g_icon_handle = 500;

HANDLE ShellGetIcon(LPCSTR lpPath, uint32_t iconFlags) {
    (void)lpPath; (void)iconFlags;
    return (HANDLE)(g_icon_handle++);
}
