#include "../include/kernel32_api.h"
#include "kernel/core/lib/include/string.h"

static char g_current_dir[260] = "C:\\";

BOOL CreateDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) {
    (void)lpPathName; (void)lpSecurityAttributes;
    return true;
}

BOOL RemoveDirectory(LPCSTR lpPathName) {
    (void)lpPathName;
    return true;
}

HANDLE FindFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) {
    (void)lpFileName;
    if (!lpFindFileData) return INVALID_HANDLE_VALUE;
    lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    strcpy(lpFindFileData->cFileName, "file1.txt");
    return (HANDLE)1;
}

BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData) {
    (void)hFindFile; (void)lpFindFileData;
    return false; // End of iteration
}

DWORD GetCurrentDirectory(DWORD nBufferLength, LPSTR lpBuffer) {
    if (!lpBuffer || nBufferLength < 4) return 0;
    strcpy(lpBuffer, g_current_dir);
    return (DWORD)strlen(g_current_dir);
}

BOOL SetCurrentDirectory(LPCSTR lpPathName) {
    if (!lpPathName) return false;
    strcpy(g_current_dir, lpPathName);
    return true;
}
