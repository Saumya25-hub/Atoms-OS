#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

bool GetMessage(MSG* lpMsg, HWND hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax) {
    (void)wMsgFilterMin; (void)wMsgFilterMax;
    if (!lpMsg) return false;
    
    BARMessage bar_msg;
    if (BAR_PeekMessage(hWnd, &bar_msg)) {
        lpMsg->hwnd = bar_msg.window_id;
        lpMsg->message = bar_msg.type;
        lpMsg->wParam = bar_msg.param1;
        lpMsg->lParam = bar_msg.param2;
        lpMsg->time = (uint32_t)bar_msg.timestamp;
        return true;
    }
    
    lpMsg->message = WM_NULL;
    return true;
}

bool PeekMessage(MSG* lpMsg, HWND hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax, uint32_t wRemoveMsg) {
    (void)wRemoveMsg;
    return GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

bool TranslateMessage(const MSG* lpMsg) {
    (void)lpMsg;
    return true;
}

LRESULT DispatchMessage(const MSG* lpMsg) {
    if (!lpMsg) return 0;
    BARMessage bar_msg = {lpMsg->hwnd, lpMsg->message, (uint32_t)lpMsg->wParam, (uint32_t)lpMsg->lParam, lpMsg->time};
    return BAR_DispatchMessage(&bar_msg);
}

LRESULT SendMessage(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam) {
    return BAR_SendMessage(hWnd, Msg, (uint32_t)wParam, (uint32_t)lParam);
}

bool PostMessage(HWND hWnd, uint32_t Msg, WPARAM wParam, LPARAM lParam) {
    return (BAR_PostMessage(hWnd, Msg, (uint32_t)wParam, (uint32_t)lParam) == 0);
}

void PostQuitMessage(int32_t nExitCode) {
    BAR_PostMessage(0, WM_QUIT, (uint32_t)nExitCode, 0);
}
