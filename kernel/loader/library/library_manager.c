#include "library_manager.h"
#include "../debug/loader_debug.h"
#include "kernel/core/lib/include/string.h"

static shared_library_t g_library_cache[MAX_LOADED_LIBRARIES];
static uint32_t         g_library_count = 0;

void library_manager_init(void) {
    memset(g_library_cache, 0, sizeof(g_library_cache));
    g_library_count = 0;
    loader_debug_log(LOG_LEVEL_INFO, "LIB_MANAGER", "Shared Library Infrastructure & Cache Initialized");
}

shared_library_t* library_manager_find(const char* name) {
    if (!name) return NULL;

    for (uint32_t i = 0; i < g_library_count; i++) {
        if (g_library_cache[i].is_loaded && strcmp(g_library_cache[i].name, name) == 0) {
            return &g_library_cache[i];
        }
    }
    return NULL;
}

shared_library_t* library_manager_load(const char* name, int flags) {
    (void)flags;
    if (!name) return NULL;

    shared_library_t* existing = library_manager_find(name);
    if (existing) {
        existing->ref_count++;
        loader_debug_log(LOG_LEVEL_INFO, "LIB_MANAGER", "Reusing already loaded Shared Library from cache");
        return existing;
    }

    if (g_library_count >= MAX_LOADED_LIBRARIES) {
        loader_debug_log(LOG_LEVEL_ERROR, "LIB_MANAGER", "Maximum shared library cache limit reached");
        return NULL;
    }

    shared_library_t* lib = &g_library_cache[g_library_count++];
    uint32_t idx = 0;
    while (name[idx] && idx < sizeof(lib->name) - 1) {
        lib->name[idx] = name[idx];
        idx++;
    }
    lib->name[idx] = '\0';
    lib->base_address = 0x7FF000000000ULL + (g_library_count * 0x1000000ULL);
    lib->image_size = 0x100000;
    lib->ref_count = 1;
    lib->handle = (library_handle_t)lib;
    lib->is_loaded = true;

    loader_debug_log(LOG_LEVEL_INFO, "LIB_MANAGER", "Successfully loaded & cached new Shared Library");
    return lib;
}

loader_status_t library_manager_unload(shared_library_t* lib) {
    if (!lib || !lib->is_loaded) return LOADER_ERR_NULL_POINTER;

    if (lib->ref_count > 1) {
        lib->ref_count--;
        return LOADER_SUCCESS;
    }

    lib->is_loaded = false;
    lib->ref_count = 0;
    loader_debug_log(LOG_LEVEL_INFO, "LIB_MANAGER", "Shared Library reference count reached zero -> Unloaded");
    return LOADER_SUCCESS;
}
