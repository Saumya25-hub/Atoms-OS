/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Dynamic Loading Implementation
 */

#include "apal_loader.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <string.h>

#define MAX_LOADED_MODULES 16

typedef struct {
    const char *name;
    void *address;
} apal_symbol_entry_t;

typedef struct {
    bool in_use;
    char path[128];
    void *base_address;
    size_t size;
    apal_symbol_entry_t symbols[32];
    size_t symbol_count;
} apal_module_entry_t;

static apal_module_entry_t g_modules[MAX_LOADED_MODULES];
static bool g_loader_init = false;

static void ensure_loader_init(void) {
    if (!g_loader_init) {
        memset(g_modules, 0, sizeof(g_modules));
        g_loader_init = true;
    }
}

apal_status_t apal_dlopen(const char *path, apal_module_handle_t *out_module) {
    if (!path || !out_module) return APAL_ERR_INVALID_PARAM;
    ensure_loader_init();

    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (!g_modules[i].in_use) {
            g_modules[i].in_use = true;
            strncpy(g_modules[i].path, path, sizeof(g_modules[i].path) - 1);
            g_modules[i].base_address = (void *)0x60000000ULL;
            g_modules[i].symbol_count = 0;

            *out_module = (apal_module_handle_t)(i + 1);
            return APAL_OK;
        }
    }
    return APAL_ERR_NO_MEMORY;
}

void *apal_dlsym(apal_module_handle_t module, const char *symbol_name) {
    if (module == 0 || !symbol_name) return NULL;
    int idx = (int)(module - 1);
    if (idx < 0 || idx >= MAX_LOADED_MODULES || !g_modules[idx].in_use) {
        return NULL;
    }

    apal_module_entry_t *m = &g_modules[idx];
    for (size_t i = 0; i < m->symbol_count; i++) {
        if (strcmp(m->symbols[i].name, symbol_name) == 0) {
            return m->symbols[i].address;
        }
    }
    return NULL;
}

apal_status_t apal_dlclose(apal_module_handle_t module) {
    if (module == 0) return APAL_ERR_INVALID_PARAM;
    int idx = (int)(module - 1);
    if (idx < 0 || idx >= MAX_LOADED_MODULES || !g_modules[idx].in_use) {
        return APAL_ERR_NOT_FOUND;
    }

    g_modules[idx].in_use = false;
    g_modules[idx].symbol_count = 0;
    return APAL_OK;
}
