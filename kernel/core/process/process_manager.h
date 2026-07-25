#ifndef ATOMS_PROCESS_MANAGER_H
#define ATOMS_PROCESS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Enterprise Process Manager (Phase 10)
// ============================================================

#define ATOMS_MAX_PROCESSES         64
#define ATOMS_MAX_THREADS_PER_PROC  16
#define ATOMS_MAX_WINDOWS_PER_PROC  16

typedef enum {
    ATOMS_PROC_STATE_CLOSED = 0,
    ATOMS_PROC_STATE_CREATING,
    ATOMS_PROC_STATE_READY,
    ATOMS_PROC_STATE_RUNNING,
    ATOMS_PROC_STATE_SUSPENDED,
    ATOMS_PROC_STATE_ZOMBIE,
    ATOMS_PROC_STATE_TERMINATED
} ATOMS_ProcessState;

typedef struct {
    uint32_t           pid;                     // Process Identifier
    uint32_t           parent_pid;              // Parent Process ID
    char               name[64];                // Executable Name
    char               filepath[128];           // Full Executable Path
    char               working_dir[128];        // Current Working Directory
    
    ATOMS_ProcessState state;                   // Process Lifecycle State
    uint64_t           pml4_phys;               // Virtual Memory Address Space Page Directory
    uint64_t           heap_base;               // Process Private Heap Virtual Base
    uint32_t           heap_size;               // Allocated Heap Size in Bytes
    uint64_t           stack_base;              // Main Thread Stack Virtual Base
    uint32_t           stack_size;              // Main Thread Stack Size in Bytes
    
    uint32_t           thread_ids[ATOMS_MAX_THREADS_PER_PROC]; // Owned Threads
    uint32_t           thread_count;            // Count of Active Threads
    
    uint32_t           window_ids[ATOMS_MAX_WINDOWS_PER_PROC]; // Owned Window Surfaces
    uint32_t           window_count;            // Count of Active Windows
    
    uint32_t           capabilities_mask;       // Security Capabilities Bitmap
    uint32_t           cpu_time_ms;             // Accumulated CPU Usage
    uint32_t           memory_used_bytes;       // Total Memory Usage
    int32_t            exit_code;               // Process Exit Code
    uint32_t           ref_count;               // Reference Count
} ATOMS_ProcessControlBlock;

typedef ATOMS_ProcessControlBlock ATOMS_PCB;

void       ATOMS_ProcessManager_Init(void);
ATOMS_PCB* ATOMS_Process_Create(const char* name, const char* filepath, uint32_t parent_pid, uint32_t capabilities);
bool       ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
ATOMS_PCB* ATOMS_Process_GetByPID(uint32_t pid);
uint32_t   ATOMS_Process_GetCount(void);
void       ATOMS_Process_DumpTelemetry(void);

// Centralized PID Manager & Traverser APIs (Phase 1)
uint32_t   ATOMS_PID_Alloc(void);
void       ATOMS_PID_Free(uint32_t pid);
ATOMS_PCB* ATOMS_Process_GetByIndex(uint32_t index);
int32_t    ATOMS_Process_Wait(uint32_t pid, int32_t* out_exit_code);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_PROCESS_MANAGER_H
