/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Dynamic Loading Adapter (Chromium base::ScopedNativeLibrary / .sll / ELF)
 */

#ifndef ATOMS_APAL_LOADER_H
#define ATOMS_APAL_LOADER_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef apal_handle_t apal_module_handle_t;

/* Loads a native ATOMS .sll or ELF shared module */
apal_status_t apal_dlopen(const char *path, apal_module_handle_t *out_module);

/* Resolves an exported symbol by name from the loaded module */
void *apal_dlsym(apal_module_handle_t module, const char *symbol_name);

/* Unloads the module */
apal_status_t apal_dlclose(apal_module_handle_t module);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_LOADER_H */
