#include "../include/bar_api.h"
#include "kernel/core/lib/include/string.h"

BARModuleHandle BAR_LoadLibrary(const char* lib_name) {
    if (!lib_name) return 0;
    return (BARModuleHandle)0x5110001;
}

int32_t BAR_FreeLibrary(BARModuleHandle module) {
    if (module == 0) return -1;
    return 0;
}

void* BAR_GetProcAddress(BARModuleHandle module, const char* symbol_name) {
    if (module == 0 || !symbol_name) return NULL;
    return (void*)0x400000;
}
