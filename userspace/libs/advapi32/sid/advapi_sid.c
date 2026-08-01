#include "../include/advapi32_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

BOOL AllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD dwSubAuthority0, DWORD dwSubAuthority1, DWORD dwSubAuthority2, DWORD dwSubAuthority3, DWORD dwSubAuthority4, DWORD dwSubAuthority5, DWORD dwSubAuthority6, DWORD dwSubAuthority7, PSID* pSid) {
    (void)pIdentifierAuthority; (void)nSubAuthorityCount; (void)dwSubAuthority0; (void)dwSubAuthority1; (void)dwSubAuthority2; (void)dwSubAuthority3; (void)dwSubAuthority4; (void)dwSubAuthority5; (void)dwSubAuthority6; (void)dwSubAuthority7;
    if (!pSid) return FALSE;
    *pSid = kmalloc(32);
    return (*pSid != NULL) ? TRUE : FALSE;
}

BOOL EqualSid(PSID pSid1, PSID pSid2) {
    (void)pSid1; (void)pSid2;
    return TRUE;
}

PVOID FreeSid(PSID pSid) {
    if (pSid) kfree(pSid);
    return NULL;
}

BOOL LookupAccountSidA(LPCSTR lpSystemName, PSID Sid, LPSTR Name, LPDWORD cchName, LPSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, LPDWORD peUse) {
    (void)lpSystemName; (void)Sid; (void)peUse;
    if (Name && cchName && *cchName >= 6) {
        Name[0] = 'A'; Name[1] = 'd'; Name[2] = 'm'; Name[3] = 'i'; Name[4] = 'n'; Name[5] = '\0';
    }
    if (ReferencedDomainName && cchReferencedDomainName && *cchReferencedDomainName >= 6) {
        ReferencedDomainName[0] = 'A'; ReferencedDomainName[1] = 'T'; ReferencedDomainName[2] = 'O'; ReferencedDomainName[3] = 'M'; ReferencedDomainName[4] = 'S'; ReferencedDomainName[5] = '\0';
    }
    return TRUE;
}
