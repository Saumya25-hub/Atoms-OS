#include "../include/kernel32_api.h"

static DWORD g_thread_counter = 5000;

HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, size_t dwStackSize, void* lpStartAddress, void* lpParameter, DWORD dwCreationFlags, DWORD* lpThreadId) {
    (void)lpThreadAttributes; (void)dwStackSize; (void)lpStartAddress; (void)lpParameter; (void)dwCreationFlags;
    DWORD tid = g_thread_counter++;
    if (lpThreadId) *lpThreadId = tid;
    return (HANDLE)tid;
}

void ExitThread(DWORD dwExitCode) {
    (void)dwExitCode;
}

DWORD SuspendThread(HANDLE hThread) {
    (void)hThread;
    return 0;
}

DWORD ResumeThread(HANDLE hThread) {
    (void)hThread;
    return 1;
}

void Sleep(DWORD dwMilliseconds) {
    (void)dwMilliseconds;
}

void YieldProcessor(void) {
}
