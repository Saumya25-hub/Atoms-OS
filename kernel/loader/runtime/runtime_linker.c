#include "runtime_linker.h"
#include "../library/library_manager.h"
#include "../symbols/symbol_resolver.h"
#include "../debug/loader_debug.h"

loader_status_t runtime_linker_init(void) {
    loader_debug_log(LOG_LEVEL_INFO, "RUNTIME_LINKER", "Runtime Linker Subsystem (dlopen/dlsym/dlclose) Initialized");
    return LOADER_SUCCESS;
}

library_handle_t bos_dlopen(const char* filename, int flags) {
    if (!filename) return NULL;
    shared_library_t* lib = library_manager_load(filename, flags);
    return lib ? lib->handle : NULL;
}

void* bos_dlsym(library_handle_t handle, const char* symbol_name) {
    (void)handle;
    if (!symbol_name) return NULL;
    return symbol_resolver_lookup(symbol_name);
}

int bos_dlclose(library_handle_t handle) {
    if (!handle) return -1;
    shared_library_t* lib = (shared_library_t*)handle;
    return (library_manager_unload(lib) == LOADER_SUCCESS) ? 0 : -1;
}
