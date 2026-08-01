#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

static HCURSOR g_current_cursor = 0;

HCURSOR SetCursor(HCURSOR hCursor) {
    HCURSOR old = g_current_cursor;
    g_current_cursor = hCursor;
    return old;
}

HCURSOR LoadCursor(HINSTANCE hInstance, const char* lpCursorName) {
    (void)hInstance;
    if (!lpCursorName) return 0;
    return (HCURSOR)BAR_LoadCursor(lpCursorName);
}

HICON LoadIcon(HINSTANCE hInstance, const char* lpIconName) {
    (void)hInstance;
    if (!lpIconName) return 0;
    return (HICON)BAR_LoadIcon(lpIconName);
}
