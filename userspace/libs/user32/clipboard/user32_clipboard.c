#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

static void* g_clipboard_buffer = NULL;

bool OpenClipboard(HWND hWndNewOwner) {
    return (BAR_OpenClipboard(hWndNewOwner) == 0);
}

bool CloseClipboard(void) {
    return (BAR_CloseClipboard() == 0);
}

bool EmptyClipboard(void) {
    g_clipboard_buffer = NULL;
    return true;
}

void* SetClipboardData(uint32_t uFormat, void* hMem) {
    (void)uFormat;
    g_clipboard_buffer = hMem;
    return hMem;
}

void* GetClipboardData(uint32_t uFormat) {
    (void)uFormat;
    return g_clipboard_buffer;
}
