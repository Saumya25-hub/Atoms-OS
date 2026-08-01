#include "../include/advapi32_api.h"

BOOL AdvApiAuditSecurityEvent(DWORD dwEventId, LPCSTR lpDetails) {
    (void)dwEventId; (void)lpDetails;
    return TRUE;
}
