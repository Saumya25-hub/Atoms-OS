#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

HWND SetFocus(HWND hWnd) {
    HWND old = GetFocus();
    BAR_SetFocus(hWnd);
    return old;
}

HWND GetFocus(void) {
    return (HWND)BAR_GetFocus();
}
