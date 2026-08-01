#include "../include/advapi32_api.h"

static uintptr_t g_hkey_counter = 0x90000000;

BOOL RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, DWORD samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, HKEY* phkResult, LPDWORD lpdwDisposition) {
    (void)hKey; (void)lpSubKey; (void)Reserved; (void)lpClass; (void)dwOptions; (void)samDesired; (void)lpSecurityAttributes;
    if (phkResult) *phkResult = (HKEY)(g_hkey_counter++);
    if (lpdwDisposition) *lpdwDisposition = REG_CREATED_NEW_KEY;
    return TRUE;
}

BOOL RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY* phkResult) {
    (void)hKey; (void)lpSubKey; (void)ulOptions; (void)samDesired;
    if (phkResult) *phkResult = (HKEY)(g_hkey_counter++);
    return TRUE;
}

BOOL RegCloseKey(HKEY hKey) {
    (void)hKey;
    return TRUE;
}

BOOL RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey) {
    (void)hKey; (void)lpSubKey;
    return TRUE;
}

BOOL RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    (void)hKey; (void)lpValueName; (void)lpReserved;
    if (lpType) *lpType = REG_SZ;
    if (lpData && lpcbData && *lpcbData >= 6) {
        lpData[0] = 'A'; lpData[1] = 'T'; lpData[2] = 'O'; lpData[3] = 'M'; lpData[4] = 'S'; lpData[5] = '\0';
    }
    return TRUE;
}

BOOL RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData) {
    (void)hKey; (void)lpValueName; (void)Reserved; (void)dwType; (void)lpData; (void)cbData;
    return TRUE;
}
