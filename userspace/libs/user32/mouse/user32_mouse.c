#include "../include/user32_api.h"

static HWND g_captured_window = 0;

HWND SetCapture(HWND hWnd) {
    HWND old = g_captured_window;
    g_captured_window = hWnd;
    return old;
}

bool ReleaseCapture(void) {
    g_captured_window = 0;
    return true;
}
