#include "../include/user32_api.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    char        class_name[64];
    WNDPROC     wndproc;
    HINSTANCE   instance;
    bool        registered;
} USER32ClassEntry;

static USER32ClassEntry g_class_table[32];

uint16_t RegisterClass(const WNDCLASS* lpWndClass) {
    if (!lpWndClass || !lpWndClass->lpszClassName) return 0;
    
    for (int i = 0; i < 32; i++) {
        if (!g_class_table[i].registered) {
            strcpy(g_class_table[i].class_name, lpWndClass->lpszClassName);
            g_class_table[i].wndproc = lpWndClass->lpfnWndProc;
            g_class_table[i].instance = lpWndClass->hInstance;
            g_class_table[i].registered = true;
            return (uint16_t)(i + 1);
        }
    }
    return 0;
}

uint16_t RegisterClassEx(const WNDCLASSEX* lpwcx) {
    if (!lpwcx || !lpwcx->lpszClassName) return 0;
    WNDCLASS wc;
    wc.style = lpwcx->style;
    wc.lpfnWndProc = lpwcx->lpfnWndProc;
    wc.cbClsExtra = lpwcx->cbClsExtra;
    wc.cbWndExtra = lpwcx->cbWndExtra;
    wc.hInstance = lpwcx->hInstance;
    wc.hIcon = lpwcx->hIcon;
    wc.hCursor = lpwcx->hCursor;
    wc.hbrBackground = lpwcx->hbrBackground;
    wc.lpszMenuName = lpwcx->lpszMenuName;
    wc.lpszClassName = lpwcx->lpszClassName;
    return RegisterClass(&wc);
}
