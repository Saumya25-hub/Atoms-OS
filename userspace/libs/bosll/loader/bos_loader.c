#include "../include/bosll_api.h"

static uint64_t g_mod_handle = 1000;

BOS_HANDLE BosLoadLibrary(const char* libraryPath) {
    (void)libraryPath;
    return (BOS_HANDLE)(g_mod_handle++);
}

void* BosGetProcedure(BOS_HANDLE hModule, const char* procName) {
    (void)hModule; (void)procName;
    return (void*)0x400000;
}
