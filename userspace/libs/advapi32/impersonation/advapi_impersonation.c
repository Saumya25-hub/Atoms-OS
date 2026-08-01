#include "../include/advapi32_api.h"

BOOL ImpersonateLoggedOnUser(HANDLE hToken) {
    (void)hToken;
    return TRUE;
}

BOOL RevertToSelf(void) {
    return TRUE;
}
