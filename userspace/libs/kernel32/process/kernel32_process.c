#include "../include/kernel32_api.h"
#include "kernel/bar/include/bar_api.h"

static DWORD g_pid_counter = 1000;

BOOL CreateProcess(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) {
    (void)lpCommandLine; (void)lpProcessAttributes; (void)lpThreadAttributes; (void)bInheritHandles;
    (void)dwCreationFlags; (void)lpEnvironment; (void)lpCurrentDirectory; (void)lpStartupInfo;
    
    if (!lpApplicationName) return false;
    
    BARProcessID bar_pid = BAR_CreateProcess(lpApplicationName, lpApplicationName);
    if (bar_pid == 0) return false;
    
    if (lpProcessInformation) {
        lpProcessInformation->hProcess = (HANDLE)bar_pid;
        lpProcessInformation->hThread = (HANDLE)(bar_pid + 100);
        lpProcessInformation->dwProcessId = g_pid_counter++;
        lpProcessInformation->dwThreadId = lpProcessInformation->dwProcessId + 5000;
    }
    return true;
}

void ExitProcess(DWORD uExitCode) {
    (void)uExitCode;
}

BOOL TerminateProcess(HANDLE hProcess, DWORD uExitCode) {
    (void)uExitCode;
    if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE) return false;
    return (BAR_DestroyProcess((BARProcessID)hProcess) == 0);
}

HANDLE GetCurrentProcess(void) {
    return (HANDLE)1;
}

BOOL DuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, HANDLE* lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions) {
    (void)hSourceProcessHandle; (void)hTargetProcessHandle; (void)dwDesiredAccess; (void)bInheritHandle; (void)dwOptions;
    if (!lpTargetHandle) return false;
    *lpTargetHandle = hSourceHandle;
    return true;
}

HANDLE OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
    (void)dwDesiredAccess; (void)bInheritHandle;
    return (HANDLE)dwProcessId;
}
