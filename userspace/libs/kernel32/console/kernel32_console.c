#include "../include/kernel32_api.h"
#include "kernel/drivers/display/display.h"

HANDLE CreateConsole(void) {
    return (HANDLE)1;
}

BOOL WriteConsole(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite, DWORD* lpNumberOfCharsWritten, LPVOID lpReserved) {
    (void)hConsoleOutput; (void)lpReserved;
    if (!lpBuffer) return false;
    display_print((const char*)lpBuffer);
    if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
    return true;
}

BOOL ReadConsole(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead, DWORD* lpNumberOfCharsRead, LPVOID pInputControl) {
    (void)hConsoleInput; (void)lpBuffer; (void)nNumberOfCharsToRead; (void)pInputControl;
    if (lpNumberOfCharsRead) *lpNumberOfCharsRead = 0;
    return true;
}

BOOL SetConsoleTitle(LPCSTR lpConsoleTitle) {
    (void)lpConsoleTitle;
    return true;
}
