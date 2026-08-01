#include "../include/shell32_api.h"

HINSTANCE ShellExecute(HANDLE hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd) {
    (void)hwnd; (void)lpOperation; (void)lpFile; (void)lpParameters; (void)lpDirectory; (void)nShowCmd;
    return (HINSTANCE)32; // Standard HINSTANCE success >= 32
}

BOOL ShellExecuteEx(SHELLEXECUTEINFO* pExecInfo) {
    if (!pExecInfo) return false;
    pExecInfo->hInstApp = (HINSTANCE)32;
    return true;
}
