#ifndef BOS_LIBRARY_MANAGER_H
#define BOS_LIBRARY_MANAGER_H

#include "../include/loader_types.h"

#define MAX_LOADED_LIBRARIES 32

typedef struct {
    char            name[128];
    uint64_t        base_address;
    uint64_t        image_size;
    uint32_t        ref_count;
    library_handle_t handle;
    bool            is_loaded;
} shared_library_t;

void             library_manager_init(void);
shared_library_t* library_manager_find(const char* name);
shared_library_t* library_manager_load(const char* name, int flags);
loader_status_t  library_manager_unload(shared_library_t* lib);

#endif // BOS_LIBRARY_MANAGER_H
