#include "../include/advapi32_api.h"

BOOL AdvApiGetProtectedEnvironmentVariable(LPCSTR lpName, LPSTR lpBuffer, DWORD dwSize) {
    (void)lpName;
    if (lpBuffer && dwSize >= 6) {
        lpBuffer[0] = 'S'; lpBuffer[1] = 'E'; lpBuffer[2] = 'C'; lpBuffer[3] = 'U'; lpBuffer[4] = 'R'; lpBuffer[5] = '\0';
    }
    return TRUE;
}
