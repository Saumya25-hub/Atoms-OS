#include "../include/ole32_api.h"

extern void* kmalloc(size_t size);

HRESULT CoCreateGuid(GUID* pguid) {
    if (!pguid) return E_POINTER;
    pguid->Data1 = 0x12345678;
    pguid->Data2 = 0xABCD;
    pguid->Data3 = 0xEF01;
    pguid->Data4[0] = 0x23; pguid->Data4[1] = 0x45; pguid->Data4[2] = 0x67; pguid->Data4[3] = 0x89;
    pguid->Data4[4] = 0xAB; pguid->Data4[5] = 0xCD; pguid->Data4[6] = 0xEF; pguid->Data4[7] = 0x01;
    return S_OK;
}

HRESULT StringFromCLSID(REFCLSID rclsid, char** lplpsz) {
    (void)rclsid;
    if (!lplpsz) return E_POINTER;
    char* str = (char*)kmalloc(39);
    if (!str) return E_OUTOFMEMORY;
    const char* def = "{12345678-ABCD-EF01-2345-6789ABCDEF01}";
    for (int i = 0; i < 38; i++) str[i] = def[i];
    str[38] = '\0';
    *lplpsz = str;
    return S_OK;
}

HRESULT CLSIDFromString(const char* lpsz, CLSID* pclsid) {
    (void)lpsz;
    if (!pclsid) return E_POINTER;
    return CoCreateGuid(pclsid);
}
