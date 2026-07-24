#include "loader_manager.h"
#include "../debug/loader_debug.h"
#include "../symbols/symbol_resolver.h"
#include "../reloc/reloc_engine.h"
#include "../segment/segment_loader.h"
#include "../library/library_manager.h"
#include "../runtime/runtime_linker.h"

static bool g_loader_initialized = false;

loader_status_t bos_loader_init(void) {
    return loader_manager_init();
}

loader_status_t bos_loader_shutdown(void) {
    return loader_manager_shutdown();
}

loader_status_t loader_manager_init(void) {
    if (g_loader_initialized) return LOADER_SUCCESS;

    loader_debug_init(LOG_LEVEL_INFO);
    loader_debug_log(LOG_LEVEL_INFO, "LOADER_MGR", "Initializing BOS OS Phase 1 Production Dynamic Loader...");

    symbol_resolver_init();
    reloc_engine_init();
    segment_loader_init();
    library_manager_init();
    runtime_linker_init();

    g_loader_initialized = true;
    loader_debug_log(LOG_LEVEL_INFO, "LOADER_MGR", "BOS OS Dynamic Loader Subsystem Ready & Active");
    return LOADER_SUCCESS;
}

loader_status_t loader_manager_shutdown(void) {
    if (!g_loader_initialized) return LOADER_SUCCESS;
    g_loader_initialized = false;
    loader_debug_log(LOG_LEVEL_INFO, "LOADER_MGR", "BOS OS Dynamic Loader Subsystem Shutdown Cleanly");
    return LOADER_SUCCESS;
}
