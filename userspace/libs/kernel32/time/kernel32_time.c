#include "../include/kernel32_api.h"

static DWORD g_simulated_tick = 100;

DWORD GetTickCount(void) {
    g_simulated_tick += 10;
    return g_simulated_tick;
}

BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    if (!lpPerformanceCount) return false;
    lpPerformanceCount->QuadPart = (int64_t)GetTickCount() * 1000LL;
    return true;
}

BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) {
    if (!lpFrequency) return false;
    lpFrequency->QuadPart = 1000000LL; // 1 MHz
    return true;
}
