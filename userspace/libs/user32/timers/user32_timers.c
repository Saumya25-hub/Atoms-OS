#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

uint32_t SetTimer(HWND hWnd, uint32_t nIDEvent, uint32_t uElapse, void* lpTimerFunc) {
    (void)nIDEvent; (void)lpTimerFunc;
    return (uint32_t)BAR_SetTimer(hWnd, uElapse);
}

bool KillTimer(HWND hWnd, uint32_t uIDEvent) {
    (void)hWnd;
    return (BAR_KillTimer(uIDEvent) == 0);
}
