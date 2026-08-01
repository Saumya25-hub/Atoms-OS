#include "../include/user32_api.h"

uint32_t SetWindowsHookEx(int32_t idHook, void* lpfn, HINSTANCE hMod, uint32_t dwThreadId) {
    (void)idHook; (void)lpfn; (void)hMod; (void)dwThreadId;
    return 1;
}

bool UnhookWindowsHookEx(uint32_t hhk) {
    (void)hhk;
    return true;
}
