#ifndef BOS_LOADER_MANAGER_H
#define BOS_LOADER_MANAGER_H

#include "../include/loader_types.h"
#include "../include/loader_api.h"

loader_status_t loader_manager_init(void);
loader_status_t loader_manager_shutdown(void);

#endif // BOS_LOADER_MANAGER_H
