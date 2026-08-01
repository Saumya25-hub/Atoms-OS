#include "../include/kernel32_api.h"

BOOL CreatePipe(HANDLE* hReadPipe, HANDLE* hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize) {
    (void)lpPipeAttributes; (void)nSize;
    if (!hReadPipe || !hWritePipe) return false;
    *hReadPipe = (HANDLE)101;
    *hWritePipe = (HANDLE)102;
    return true;
}
