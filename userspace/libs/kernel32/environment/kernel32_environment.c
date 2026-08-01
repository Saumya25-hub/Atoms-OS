#include "../include/kernel32_api.h"
#include "kernel/core/lib/include/string.h"

static char g_cmdline[260] = "ATOMS.exe";
static char g_env_path[260] = "C:\\System";

BOOL SetEnvironmentVariable(LPCSTR lpName, LPCSTR lpValue) {
    if (!lpName) return false;
    if (!lpValue) return true;
    if (strcmp(lpName, "PATH") == 0) strcpy(g_env_path, lpValue);
    return true;
}

DWORD GetEnvironmentVariable(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize) {
    if (!lpName || !lpBuffer || nSize == 0) return 0;
    if (strcmp(lpName, "PATH") == 0) {
        strcpy(lpBuffer, g_env_path);
        return (DWORD)strlen(g_env_path);
    }
    lpBuffer[0] = '\0';
    return 0;
}

LPSTR GetCommandLine(void) {
    return g_cmdline;
}
