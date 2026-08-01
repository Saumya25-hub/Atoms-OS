#include "../include/advapi32_api.h"

static uintptr_t g_sc_handle_counter = 0x50000000;

SC_HANDLE OpenSCManagerA(LPCSTR lpMachineName, LPCSTR lpDatabaseName, DWORD dwDesiredAccess) {
    (void)lpMachineName; (void)lpDatabaseName; (void)dwDesiredAccess;
    return (SC_HANDLE)(g_sc_handle_counter++);
}

SC_HANDLE CreateServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, LPCSTR lpDisplayName, DWORD dwDesiredAccess, DWORD dwServiceType, DWORD dwStartType, DWORD dwErrorControl, LPCSTR lpBinaryPathName, LPCSTR lpLoadOrderGroup, LPDWORD lpdwTagId, LPCSTR lpDependencies, LPCSTR lpServiceStartName, LPCSTR lpPassword) {
    (void)hSCManager; (void)lpServiceName; (void)lpDisplayName; (void)dwDesiredAccess; (void)dwServiceType; (void)dwStartType; (void)dwErrorControl; (void)lpBinaryPathName; (void)lpLoadOrderGroup; (void)lpdwTagId; (void)lpDependencies; (void)lpServiceStartName; (void)lpPassword;
    return (SC_HANDLE)(g_sc_handle_counter++);
}

BOOL StartServiceA(SC_HANDLE hService, DWORD dwNumServiceArgs, LPCSTR* lpServiceArgVectors) {
    (void)hService; (void)dwNumServiceArgs; (void)lpServiceArgVectors;
    return TRUE;
}

BOOL ControlService(SC_HANDLE hService, DWORD dwControl, PVOID lpServiceStatus) {
    (void)hService; (void)dwControl; (void)lpServiceStatus;
    return TRUE;
}

BOOL DeleteService(SC_HANDLE hService) {
    (void)hService;
    return TRUE;
}

BOOL CloseServiceHandle(SC_HANDLE hSCObject) {
    (void)hSCObject;
    return TRUE;
}
