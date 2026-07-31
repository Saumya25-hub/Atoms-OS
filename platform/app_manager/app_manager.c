#include "platform/include/bos_app_manager.h"
#include "kernel/core/lib/include/string.h"

static BOS_AppInstance g_app_instances[BOS_MAX_APP_INSTANCES];

void BOS_AppManager_Init(void) {
    memset(g_app_instances, 0, sizeof(g_app_instances));
}

BOS_Result BOS_AppManager_RegisterProcess(uint32_t pid, const char* name, uint32_t* out_app_id) {
    if (pid == 0 || !name || !out_app_id) return BOS_ERROR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOS_MAX_APP_INSTANCES; i++) {
        if (!g_app_instances[i].is_active) {
            g_app_instances[i].app_id = i + 1;
            g_app_instances[i].owner_pid = pid;
            strncpy(g_app_instances[i].name, name, 63);
            g_app_instances[i].name[63] = '\0';
            g_app_instances[i].active_window_count = 0;
            g_app_instances[i].permissions_mask = 0xFFFFFFFFU;
            g_app_instances[i].is_active = true;

            *out_app_id = g_app_instances[i].app_id;
            return BOS_SUCCESS;
        }
    }
    return BOS_ERROR_OUT_OF_MEMORY;
}

BOS_Result BOS_AppManager_UnregisterProcess(uint32_t pid) {
    for (uint32_t i = 0; i < BOS_MAX_APP_INSTANCES; i++) {
        if (g_app_instances[i].is_active && g_app_instances[i].owner_pid == pid) {
            BOS_AppManager_CleanupOrphanWindows(pid);
            g_app_instances[i].is_active = false;
            return BOS_SUCCESS;
        }
    }
    return BOS_ERROR_NOT_FOUND;
}

BOS_Result BOS_AppManager_FindInstance(uint32_t pid, BOS_AppInstance* out_instance) {
    if (!out_instance) return BOS_ERROR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < BOS_MAX_APP_INSTANCES; i++) {
        if (g_app_instances[i].is_active && g_app_instances[i].owner_pid == pid) {
            *out_instance = g_app_instances[i];
            return BOS_SUCCESS;
        }
    }
    return BOS_ERROR_NOT_FOUND;
}
