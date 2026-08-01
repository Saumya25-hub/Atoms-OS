#include "../include/advapi32_api.h"

static uintptr_t g_crypto_counter = 0x40000000;

BOOL CryptAcquireContextA(HCRYPTPROV* phProv, LPCSTR pszContainer, LPCSTR pszProvider, DWORD dwProvType, DWORD dwFlags) {
    (void)pszContainer; (void)pszProvider; (void)dwProvType; (void)dwFlags;
    if (phProv) *phProv = (HCRYPTPROV)(g_crypto_counter++);
    return TRUE;
}

BOOL CryptGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer) {
    (void)hProv;
    if (!pbBuffer) return FALSE;
    for (DWORD i = 0; i < dwLen; i++) {
        pbBuffer[i] = (BYTE)(i * 37 + 13);
    }
    return TRUE;
}

BOOL CryptCreateHash(HCRYPTPROV hProv, DWORD Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH* phHash) {
    (void)hProv; (void)Algid; (void)hKey; (void)dwFlags;
    if (phHash) *phHash = (HCRYPTHASH)(g_crypto_counter++);
    return TRUE;
}

BOOL CryptHashData(HCRYPTHASH hHash, const BYTE* pbData, DWORD dwDataLen, DWORD dwFlags) {
    (void)hHash; (void)pbData; (void)dwDataLen; (void)dwFlags;
    return TRUE;
}

BOOL CryptEncrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE* pbData, DWORD* pdwDataLen, DWORD dwBufLen) {
    (void)hKey; (void)hHash; (void)Final; (void)dwFlags; (void)pbData; (void)pdwDataLen; (void)dwBufLen;
    return TRUE;
}

BOOL CryptDecrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, BYTE* pbData, DWORD* pdwDataLen) {
    (void)hKey; (void)hHash; (void)Final; (void)dwFlags; (void)pbData; (void)pdwDataLen;
    return TRUE;
}
