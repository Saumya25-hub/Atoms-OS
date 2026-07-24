#ifndef BOS_LOADER_API_H
#define BOS_LOADER_API_H

#include "loader_types.h"

// Public Dynamic Linker API (POSIX Equivalent)
library_handle_t bos_dlopen(const char* filename, int flags);
void*            bos_dlsym(library_handle_t handle, const char* symbol_name);
int              bos_dlclose(library_handle_t handle);
const char*      bos_dlerror(void);

// Core Loader Subsystem Lifecycle
loader_status_t bos_loader_init(void);
loader_status_t bos_loader_shutdown(void);

#endif // BOS_LOADER_API_H
