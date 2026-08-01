#include "../include/advapi32_api.h"

BOOL AdvApiValidateHandleSecurity(HANDLE hHandle, DWORD dwAccessMask) {
    (void)hHandle; (void)dwAccessMask;
    return TRUE;
}
