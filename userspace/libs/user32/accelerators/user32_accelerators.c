#include "../include/user32_api.h"

HACCEL CreateAcceleratorTable(LPACCEL paccel, int32_t cAccel) {
    (void)paccel; (void)cAccel;
    return (HACCEL)1;
}

int32_t TranslateAccelerator(HWND hWnd, HACCEL hAccel, MSG* lpMsg) {
    (void)hWnd; (void)hAccel; (void)lpMsg;
    return 0;
}
