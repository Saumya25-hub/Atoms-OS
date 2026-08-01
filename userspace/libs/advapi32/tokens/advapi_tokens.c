#include "../include/advapi32_api.h"

static HANDLE g_token_counter = (HANDLE)0x7000;

BOOL OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, HANDLE* TokenHandle) {
    (void)ProcessHandle; (void)DesiredAccess;
    if (TokenHandle) *TokenHandle = (HANDLE)(g_token_counter++);
    return TRUE;
}

BOOL OpenThreadToken(HANDLE ThreadHandle, DWORD DesiredAccess, BOOL OpenAsSelf, HANDLE* TokenHandle) {
    (void)ThreadHandle; (void)DesiredAccess; (void)OpenAsSelf;
    if (TokenHandle) *TokenHandle = (HANDLE)(g_token_counter++);
    return TRUE;
}

BOOL DuplicateToken(HANDLE ExistingTokenHandle, DWORD ImpersonationLevel, HANDLE* DuplicateTokenHandle) {
    (void)ExistingTokenHandle; (void)ImpersonationLevel;
    if (DuplicateTokenHandle) *DuplicateTokenHandle = (HANDLE)(g_token_counter++);
    return TRUE;
}
