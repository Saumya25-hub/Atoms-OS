#ifndef BOS_APP_MANAGER_H
#define BOS_APP_MANAGER_H

#include "bos_types.h"

#define BOS_MAX_APP_INSTANCES 64U

typedef struct {
    uint32_t app_id;
    uint32_t owner_pid;
    char     name[64];
    uint32_t active_window_count;
    uint32_t permissions_mask;
    bool     is_active;
} BOS_AppInstance;

void       BOS_AppManager_Init(void);
BOS_Result BOS_AppManager_RegisterProcess(uint32_t pid, const char* name, uint32_t* out_app_id);
BOS_Result BOS_AppManager_UnregisterProcess(uint32_t pid);
void       BOS_AppManager_CleanupOrphanWindows(uint32_t pid);
BOS_Result BOS_AppManager_FindInstance(uint32_t pid, BOS_AppInstance* out_instance);

#endif /* BOS_APP_MANAGER_H */
