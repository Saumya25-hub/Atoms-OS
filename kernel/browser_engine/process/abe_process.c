#include "abe_process.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_ProcessManager g_proc_mgr;
static uint32_t g_next_pid = 1000;

ABE_Error ABE_Process_Init(void) {
    memset(&g_proc_mgr, 0, sizeof(ABE_ProcessManager));
    g_proc_mgr.is_active = true;

    // Register main browser process
    uint32_t main_pid = 0;
    ABE_Error err = ABE_Process_Create(ABE_PROC_ROLE_BROWSER_MAIN, &main_pid);
    if (err == ABE_SUCCESS) {
        g_proc_mgr.main_browser_pid = main_pid;
    }
    ABE_Log(ABE_LOG_INFO, "PROC", "ABE Browser Process Manager initialized with main browser process PID");
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Shutdown(void) {
    if (!g_proc_mgr.is_active) return ABE_ERR_NOT_INITIALIZED;
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].state == ABE_PROC_STATE_RUNNING) {
            g_proc_mgr.processes[i].state = ABE_PROC_STATE_TERMINATED;
        }
    }
    g_proc_mgr.is_active = false;
    ABE_Log(ABE_LOG_INFO, "PROC", "ABE Process Manager shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Create(ABE_ProcessRole role, uint32_t* out_pid) {
    if (!g_proc_mgr.is_active || !out_pid) return ABE_ERR_INVALID_PARAM;
    if (g_proc_mgr.process_count >= ABE_MAX_PROCESSES) return ABE_ERR_RESOURCE_EXHAUSTED;

    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].state == ABE_PROC_STATE_UNINIT ||
            g_proc_mgr.processes[i].state == ABE_PROC_STATE_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_ProcessNode* node = &g_proc_mgr.processes[slot];
    memset(node, 0, sizeof(ABE_ProcessNode));
    node->process_id = g_next_pid++;
    node->role = role;
    node->state = ABE_PROC_STATE_RUNNING;
    node->crash_safe_cleanup = true;
    node->start_timestamp = 100;

    g_proc_mgr.process_count++;
    *out_pid = node->process_id;
    ABE_LogVal(ABE_LOG_INFO, "PROC", "Created process PID: ", node->process_id);
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Terminate(uint32_t pid) {
    if (!g_proc_mgr.is_active || pid == 0) return ABE_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].process_id == pid &&
            g_proc_mgr.processes[i].state == ABE_PROC_STATE_RUNNING) {
            g_proc_mgr.processes[i].state = ABE_PROC_STATE_TERMINATED;
            if (g_proc_mgr.process_count > 0) g_proc_mgr.process_count--;
            ABE_LogVal(ABE_LOG_INFO, "PROC", "Terminated process PID: ", pid);
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_Process_CrashHandler(uint32_t pid) {
    if (!g_proc_mgr.is_active || pid == 0) return ABE_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].process_id == pid) {
            g_proc_mgr.processes[i].state = ABE_PROC_STATE_CRASHED;
            ABE_LogVal(ABE_LOG_ERROR, "PROC", "Process CRASH detected for PID: ", pid);
            // Emergency safe cleanup
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}
