#include "../include/advapi32_api.h"

BOOL RegNotifyChangeKeyValue(HKEY hKey, BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL fAsynchronous) {
    (void)hKey; (void)bWatchSubtree; (void)dwNotifyFilter; (void)hEvent; (void)fAsynchronous;
    return TRUE;
}
