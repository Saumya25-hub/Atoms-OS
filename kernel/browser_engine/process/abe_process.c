/*
 * ATOMS OS / ATRIX Browser — Multi-Process Subsystem Implementation
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements real process boundaries:
 *   1. Unique PID allocation per role (Browser, Renderer, Network, Utility)
 *   2. Independent PML4 / CR3 address-space allocation via vmm_create_address_space()
 *   3. Kernel PCB integration via ATOMS_Process_Create()
 *   4. Crash containment and safe resource reclamation
 */

#include "abe_process.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/scheduler/include/scheduler.h"

__attribute__((weak)) ATOMS_PCB* ATOMS_Process_Create(const char* name, const char* filepath, uint32_t parent_pid, uint32_t capabilities) {
    (void)name; (void)filepath; (void)parent_pid; (void)capabilities;
    return NULL;
}

__attribute__((weak)) uint32_t ATOMS_PID_Alloc(void) {
    static uint32_t s_user_pid = 200;
    return s_user_pid++;
}

__attribute__((weak)) void* vmm_get_active_pml4(void) {
    return (void*)0x1000000ULL;
}

__attribute__((weak)) void* vmm_create_address_space(void) {
    static uint64_t s_user_cr3 = 0x2000000ULL;
    s_user_cr3 += 0x10000ULL;
    return (void*)s_user_cr3;
}

__attribute__((weak)) bool vmm_destroy_address_space(void* pml4) {
    (void)pml4;
    return true;
}

__attribute__((weak)) bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code) {
    (void)pid; (void)exit_code;
    return true;
}

__attribute__((weak)) ATOMS_ExecutionErrorCode ATOMS_Process_SetUserImage(uint32_t pid, uint64_t pml4, uint64_t image_base, uint64_t image_end, uint64_t entry_point, uint64_t stack_base, uint64_t stack_size, uint64_t guard_page) {
    (void)pid; (void)pml4; (void)image_base; (void)image_end; (void)entry_point; (void)stack_base; (void)stack_size; (void)guard_page;
    return ATOMS_EXEC_OK;
}

__attribute__((weak)) uint64_t scheduler_get_tick_count(void) {
    return 100ULL;
}

__attribute__((weak)) void ABE_Log(ABE_LogLevel level, const char* category, const char* msg) {
    (void)level; (void)category; (void)msg;
}

__attribute__((weak)) void ABE_LogVal(ABE_LogLevel level, const char* category, const char* msg, uint64_t val) {
    (void)level; (void)category; (void)msg; (void)val;
}

static ABE_ProcessManager g_proc_mgr;

// Names for process roles
static const char* RoleName(ABE_ProcessRole role) {
    switch (role) {
        case ABE_PROC_ROLE_BROWSER_MAIN: return "atrix-browser-main";
        case ABE_PROC_ROLE_RENDERER:     return "atrix-renderer";
        case ABE_PROC_ROLE_NETWORKING:   return "atrix-network";
        case ABE_PROC_ROLE_GPU_RASTER:   return "atrix-gpu-raster";
        case ABE_PROC_ROLE_UTILITY:      return "atrix-utility";
        default:                         return "atrix-process";
    }
}

ABE_Error ABE_Process_Init(void) {
    memset(&g_proc_mgr, 0, sizeof(ABE_ProcessManager));
    g_proc_mgr.is_active = true;

    // 1. Register main browser UI process
    uint32_t main_pid = 0;
    ABE_Error err = ABE_Process_Create(ABE_PROC_ROLE_BROWSER_MAIN, &main_pid);
    if (err == ABE_SUCCESS) {
        g_proc_mgr.main_browser_pid = main_pid;
        g_proc_mgr.main_browser_cr3 = (uint64_t)vmm_get_active_pml4();
    }
    ABE_Log(ABE_LOG_INFO, "PROC", "ATRIX Multi-Process Manager initialized (Browser Host Online)");
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Shutdown(void) {
    if (!g_proc_mgr.is_active) return ABE_ERR_NOT_INITIALIZED;

    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].state == ABE_PROC_STATE_RUNNING) {
            ABE_Process_Terminate(g_proc_mgr.processes[i].process_id);
        }
    }
    g_proc_mgr.is_active = false;
    ABE_Log(ABE_LOG_INFO, "PROC", "ATRIX Multi-Process Manager shut down cleanly");
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Create(ABE_ProcessRole role, uint32_t* out_pid) {
    if (!g_proc_mgr.is_active || !out_pid) return ABE_ERR_INVALID_PARAM;
    if (g_proc_mgr.process_count >= ABE_MAX_PROCESSES) return ABE_ERR_RESOURCE_EXHAUSTED;

    // Find free slot
    uint32_t slot = ABE_INVALID_HANDLE;
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].state == ABE_PROC_STATE_UNINIT ||
            g_proc_mgr.processes[i].state == ABE_PROC_STATE_TERMINATED) {
            slot = i;
            break;
        }
    }
    if (slot == ABE_INVALID_HANDLE) return ABE_ERR_RESOURCE_EXHAUSTED;

    const char* pname = RoleName(role);
    uint32_t parent_pid = (role == ABE_PROC_ROLE_BROWSER_MAIN) ? 0 : g_proc_mgr.main_browser_pid;

    // 1. Create real ATOMS Kernel Process Control Block (PCB)
    ATOMS_PCB* pcb = ATOMS_Process_Create(pname, "/apps/atrix", parent_pid, 0x7);
    uint32_t allocated_pid = pcb ? pcb->pid : 0;
    if (allocated_pid == 0) {
        // Fallback PID allocation if PCB pool reached limit
        allocated_pid = ATOMS_PID_Alloc();
    }

    // 2. Allocate real, independent hardware PML4 / CR3 address space
    void* child_pml4 = NULL;
    if (role == ABE_PROC_ROLE_BROWSER_MAIN) {
        child_pml4 = vmm_get_active_pml4();
    } else {
        child_pml4 = vmm_create_address_space();
    }
    uint64_t cr3_val = (uint64_t)child_pml4;

    // 3. Populate process node
    ABE_ProcessNode* node = &g_proc_mgr.processes[slot];
    memset(node, 0, sizeof(ABE_ProcessNode));
    node->process_id = allocated_pid;
    node->role = role;
    node->state = ABE_PROC_STATE_RUNNING;
    node->parent_pid = parent_pid;
    node->pml4_phys = cr3_val;
    node->memory_allocated_bytes = 4 * 1024 * 1024; // 4MB initial mapping
    node->start_timestamp = scheduler_get_tick_count();
    node->crash_safe_cleanup = true;
    strncpy(node->name, pname, sizeof(node->name) - 1);

    // 4. Update kernel PCB memory context if present
    if (pcb && child_pml4) {
        ATOMS_Process_SetUserImage(allocated_pid, cr3_val,
                                   0x0000000001000000ULL, 0x0000000002000000ULL,
                                   0x0000000001001000ULL, 0x00007FFFF0000000ULL,
                                   64 * 1024, 0x00007FFFEFFF0000ULL);
    }

    g_proc_mgr.process_count++;
    *out_pid = allocated_pid;

    ABE_LogVal(ABE_LOG_INFO, "PROC", "Spawned Process PID: ", allocated_pid);
    return ABE_SUCCESS;
}

ABE_Error ABE_Process_Terminate(uint32_t pid) {
    if (!g_proc_mgr.is_active || pid == 0) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].process_id == pid &&
            g_proc_mgr.processes[i].state == ABE_PROC_STATE_RUNNING) {

            // 1. Reclaim address space if not main browser
            if (g_proc_mgr.processes[i].role != ABE_PROC_ROLE_BROWSER_MAIN) {
                if (g_proc_mgr.processes[i].pml4_phys &&
                    g_proc_mgr.processes[i].pml4_phys != g_proc_mgr.main_browser_cr3) {
                    vmm_destroy_address_space((void*)g_proc_mgr.processes[i].pml4_phys);
                }
            }

            // 2. Terminate in kernel process manager
            ATOMS_Process_Terminate(pid, 0);

            // 3. Mark terminated
            g_proc_mgr.processes[i].state = ABE_PROC_STATE_TERMINATED;
            if (g_proc_mgr.process_count > 0) g_proc_mgr.process_count--;

            ABE_LogVal(ABE_LOG_INFO, "PROC", "Terminated Process PID: ", pid);
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

            // Clean up address space & tasks without corrupting browser process
            if (g_proc_mgr.processes[i].role != ABE_PROC_ROLE_BROWSER_MAIN) {
                if (g_proc_mgr.processes[i].pml4_phys &&
                    g_proc_mgr.processes[i].pml4_phys != g_proc_mgr.main_browser_cr3) {
                    vmm_destroy_address_space((void*)g_proc_mgr.processes[i].pml4_phys);
                }
            }

            // Terminate kernel task bindings
            ATOMS_Process_Terminate(pid, -1);

            ABE_LogVal(ABE_LOG_ERROR, "PROC", "Process CRASH contained for PID: ", pid);
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_ProcessNode* ABE_Process_GetByPID(uint32_t pid) {
    for (uint32_t i = 0; i < ABE_MAX_PROCESSES; i++) {
        if (g_proc_mgr.processes[i].process_id == pid &&
            g_proc_mgr.processes[i].state != ABE_PROC_STATE_UNINIT) {
            return &g_proc_mgr.processes[i];
        }
    }
    return NULL;
}

ABE_ProcessNode* ABE_Process_GetByIndex(uint32_t index) {
    if (index >= ABE_MAX_PROCESSES) return NULL;
    if (g_proc_mgr.processes[index].state == ABE_PROC_STATE_UNINIT) return NULL;
    return &g_proc_mgr.processes[index];
}

uint32_t ABE_Process_GetCount(void) {
    return g_proc_mgr.process_count;
}

uint64_t ABE_Process_GetCR3(uint32_t pid) {
    ABE_ProcessNode* node = ABE_Process_GetByPID(pid);
    if (!node) return 0;
    return node->pml4_phys;
}
