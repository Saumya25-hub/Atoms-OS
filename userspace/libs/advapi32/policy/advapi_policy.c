#include "../include/advapi32_api.h"

BOOL AdvApiLoadSystemPolicy(LPCSTR lpPolicyName, PVOID pBuffer, DWORD dwSize) {
    (void)lpPolicyName; (void)pBuffer; (void)dwSize;
    return TRUE;
}
