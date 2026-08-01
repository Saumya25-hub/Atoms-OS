#include "../include/advapi32_api.h"

BOOL InitializeSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision) {
    (void)pSecurityDescriptor; (void)dwRevision;
    return TRUE;
}

BOOL SetSecurityDescriptorOwner(PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID pOwner, BOOL bOwnerDefaulted) {
    (void)pSecurityDescriptor; (void)pOwner; (void)bOwnerDefaulted;
    return TRUE;
}

BOOL SetSecurityDescriptorDacl(PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted) {
    (void)pSecurityDescriptor; (void)bDaclPresent; (void)pDacl; (void)bDaclDefaulted;
    return TRUE;
}

DWORD GetNamedSecurityInfoA(LPCSTR pObjectName, DWORD ObjectType, DWORD SecurityInfo, PSID* ppsidOwner, PSID* ppsidGroup, PACL* ppDacl, PACL* ppSacl, PSECURITY_DESCRIPTOR* ppSecurityDescriptor) {
    (void)pObjectName; (void)ObjectType; (void)SecurityInfo; (void)ppsidOwner; (void)ppsidGroup; (void)ppDacl; (void)ppSacl;
    if (ppSecurityDescriptor) *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR)1;
    return 0;
}

DWORD SetNamedSecurityInfoA(LPSTR pObjectName, DWORD ObjectType, DWORD SecurityInfo, PSID psidOwner, PSID psidGroup, PACL pDacl, PACL pSacl) {
    (void)pObjectName; (void)ObjectType; (void)SecurityInfo; (void)psidOwner; (void)psidGroup; (void)pDacl; (void)pSacl;
    return 0;
}
