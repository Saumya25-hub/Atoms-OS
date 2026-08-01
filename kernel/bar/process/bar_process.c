#include "../include/bar_api.h"
#include "kernel/core/lib/include/string.h"

static BARProcessInfo g_bar_processes[BAR_MAX_PROCESSES];
static uint32_t g_process_count = 0;

BARProcessID BAR_CreateProcess(const char* path, const char* app_name) {
    if (!path || !app_name) return 0;
    
    for (uint32_t i = 0; i < BAR_MAX_PROCESSES; i++) {
        if (!g_bar_processes[i].active) {
            g_bar_processes[i].process_id = i + 1;
            strcpy(g_bar_processes[i].name, app_name);
            g_bar_processes[i].active = true;
            g_bar_processes[i].window_count = 0;
            g_bar_processes[i].security_token = 0xA5A50000 | (i + 1);
            g_process_count++;
            return g_bar_processes[i].process_id;
        }
    }
    return 0; // Pool full
}

int32_t BAR_DestroyProcess(BARProcessID pid) {
    if (pid == 0 || pid > BAR_MAX_PROCESSES) return -1;
    uint32_t idx = pid - 1;
    if (!g_bar_processes[idx].active) return -1;
    
    g_bar_processes[idx].active = false;
    g_bar_processes[idx].window_count = 0;
    if (g_process_count > 0) g_process_count--;
    return 0;
}

int32_t BAR_ShutdownApplication(BARProcessID pid) {
    return BAR_DestroyProcess(pid);
}
