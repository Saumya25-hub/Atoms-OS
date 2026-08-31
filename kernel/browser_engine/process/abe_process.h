/*
 * ATOMS OS / ATRIX Browser — Multi-Process Subsystem Header
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ABE_PROCESS_H
#define ABE_PROCESS_H

#include "../../../sdk/include/abe/abe.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_PROC_ROLE_BROWSER_MAIN = 0,
    ABE_PROC_ROLE_RENDERER = 1,
    ABE_PROC_ROLE_NETWORKING = 2,
    ABE_PROC_ROLE_GPU_RASTER = 3,
    ABE_PROC_ROLE_UTILITY = 4
} ABE_ProcessRole;

typedef enum {
    ABE_PROC_STATE_UNINIT = 0,
    ABE_PROC_STATE_RUNNING = 1,
    ABE_PROC_STATE_SUSPENDED = 2,
    ABE_PROC_STATE_CRASHED = 3,
    ABE_PROC_STATE_TERMINATED = 4
} ABE_ProcessState;

#define ABE_MAX_PROCESSES 16

typedef struct {
    uint32_t process_id;        // Real ATOMS kernel PID
    ABE_ProcessRole role;       // Subsystem responsibility
    ABE_ProcessState state;     // Lifecycle state
    uint32_t parent_pid;        // Browser UI process PID
    uint64_t pml4_phys;         // Unique Hardware CR3 page table base
    size_t memory_allocated_bytes;
    uint64_t start_timestamp;
    bool crash_safe_cleanup;
    char name[64];              // Process display name
} ABE_ProcessNode;

typedef struct {
    ABE_ProcessNode processes[ABE_MAX_PROCESSES];
    uint32_t process_count;
    uint32_t main_browser_pid;
    uint64_t main_browser_cr3;
    bool is_active;
} ABE_ProcessManager;

ABE_Error ABE_Process_Init(void);
ABE_Error ABE_Process_Shutdown(void);

// Spawns a real independent process with its own PID and CR3/PML4
ABE_Error ABE_Process_Create(ABE_ProcessRole role, uint32_t* out_pid);
ABE_Error ABE_Process_Terminate(uint32_t pid);
ABE_Error ABE_Process_CrashHandler(uint32_t pid);

// Accessors for process diagnostics & verification
ABE_ProcessNode* ABE_Process_GetByPID(uint32_t pid);
ABE_ProcessNode* ABE_Process_GetByIndex(uint32_t index);
uint32_t ABE_Process_GetCount(void);
uint64_t ABE_Process_GetCR3(uint32_t pid);

#ifdef __cplusplus
}
#endif

#endif // ABE_PROCESS_H
