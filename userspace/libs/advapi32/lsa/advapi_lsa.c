#include "../include/advapi32_api.h"

BOOL AdvApiLsaAuthenticateUser(LPCSTR lpUsername, LPCSTR lpPassword, HANDLE* phUserToken) {
    (void)lpUsername; (void)lpPassword;
    if (phUserToken) *phUserToken = (HANDLE)0x7001;
    return TRUE;
}
