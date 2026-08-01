#include "../include/bosll_api.h"

BOS_STATUS BosValidateSecurityToken(BOS_HANDLE hToken, uint32_t requiredPrivilege) {
    (void)hToken; (void)requiredPrivilege;
    return BOS_SUCCESS;
}
