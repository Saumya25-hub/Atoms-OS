#include "../include/advapi32_api.h"

BOOL InitializeAcl(PACL pAcl, DWORD nAclLength, DWORD dwAclRevision) {
    (void)pAcl; (void)nAclLength; (void)dwAclRevision;
    return TRUE;
}

BOOL AddAccessAllowedAce(PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid) {
    (void)pAcl; (void)dwAceRevision; (void)AccessMask; (void)pSid;
    return TRUE;
}

BOOL CheckTokenMembership(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember) {
    (void)TokenHandle; (void)SidToCheck;
    if (IsMember) *IsMember = TRUE;
    return TRUE;
}
