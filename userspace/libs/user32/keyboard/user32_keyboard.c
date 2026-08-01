#include "../include/user32_api.h"

bool RegisterHotKey(HWND hWnd, int32_t id, uint32_t fsModifiers, uint32_t vk) {
    (void)hWnd; (void)id; (void)fsModifiers; (void)vk;
    return true;
}

bool UnregisterHotKey(HWND hWnd, int32_t id) {
    (void)hWnd; (void)id;
    return true;
}
