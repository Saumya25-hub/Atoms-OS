#include "../include/user32_api.h"

static uint32_t g_menu_counter = 0;

HMENU CreateMenu(void) {
    g_menu_counter++;
    return (HMENU)g_menu_counter;
}

bool AppendMenu(HMENU hMenu, uint32_t uFlags, uint32_t uIDNewItem, const char* lpNewItem) {
    (void)uFlags; (void)uIDNewItem; (void)lpNewItem;
    return (hMenu != 0);
}

bool TrackPopupMenu(HMENU hMenu, uint32_t uFlags, int32_t x, int32_t y, int32_t nReserved, HWND hWnd, const RECT* prcRect) {
    (void)uFlags; (void)x; (void)y; (void)nReserved; (void)hWnd; (void)prcRect;
    return (hMenu != 0);
}
