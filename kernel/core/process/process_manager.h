#ifndef ATOMS_PROCESS_MANAGER_H
#define ATOMS_PROCESS_MANAGER_H

#include "../execution/include/execution_contract.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMS_MAX_PROCESSES 1024U
#define ATOMS_MAX_THREADS_PER_PROC 64U
#define ATOMS_MAX_WINDOWS_PER_PROC 16U
#define ATOMS_MAX_MODULES_PER_PROC 32U
#define ATOMS_PROCESS_NAME_LENGTH 64U
#define ATOMS_PROCESS_PATH_LENGTH 128U
#define ATOMS_PROCESS_ENV_SLOTS 16U
#define ATOMS_PID_MIN 200U
#define ATOMS_PID_MAX_DEFAULT 65535U
#define ATOMS_INVALID_PID 0U

typedef enum {
  ATOMS_PROC_STATE_CLOSED = 0,
  ATOMS_PROC_STATE_CREATED,
  ATOMS_PROC_STATE_CREATING = ATOMS_PROC_STATE_CREATED,
  ATOMS_PROC_STATE_READY,
  ATOMS_PROC_STATE_RUNNING,
  ATOMS_PROC_STATE_WAITING,
  ATOMS_PROC_STATE_SLEEPING,
  ATOMS_PROC_STATE_SUSPENDED,
  ATOMS_PROC_STATE_ZOMBIE,
  ATOMS_PROC_STATE_TERMINATED
} ATOMS_ProcessState;

typedef enum {
  ATOMS_PROCESS_PRIORITY_IDLE = 0,
  ATOMS_PROCESS_PRIORITY_LOW = 8,
  ATOMS_PROCESS_PRIORITY_NORMAL = 16,
  ATOMS_PROCESS_PRIORITY_HIGH = 24,
  ATOMS_PROCESS_PRIORITY_REALTIME = 31
} ATOMS_ProcessPriority;

typedef struct {
  uint64_t token_id;
  uint64_t capability_mask;
  uint32_t user_id;
  uint32_t group_id;
  uint32_t flags;
} ATOMS_ProcessSecurityContext;

typedef struct {
  uint64_t pml4_phys;
  uint64_t heap_base;
  uint64_t heap_size;
  uint64_t stack_base;
  uint64_t stack_size;
  uint64_t committed_bytes;
  uint64_t resident_bytes;
} ATOMS_ProcessMemoryContext;

typedef struct {
  void *table;
  uint32_t capacity;
  uint32_t count;
  uint32_t generation;
} ATOMS_ProcessHandleTable;

typedef struct {
  uint64_t memory_bytes;
  uint32_t handles;
  uint32_t threads;
  uint32_t kernel_objects;
  uint32_t file_handles;
  uint32_t ipc_objects;
  uint32_t windows;
} ATOMS_ProcessResourceUsage;

typedef struct {
  uint64_t user_ticks;
  uint64_t kernel_ticks;
  uint64_t total_ticks;
  uint64_t creation_tick;
  uint64_t exit_tick;
  uint64_t state_transitions;
  uint64_t invalid_transitions;
  uint32_t last_error;
  uint32_t warning_flags;
} ATOMS_ProcessDiagnostics;

typedef struct {
  uint32_t pid;
  uint32_t parent_pid;
  uint32_t generation;
  char name[ATOMS_PROCESS_NAME_LENGTH];
  char filepath[ATOMS_PROCESS_PATH_LENGTH];
  char working_dir[ATOMS_PROCESS_PATH_LENGTH];
  ATOMS_ProcessState state;
  uint8_t base_priority;
  uint8_t effective_priority;
  uint16_t flags;
  ATOMS_ProcessSecurityContext security;
  ATOMS_ProcessMemoryContext memory;
  ATOMS_ProcessHandleTable handle_table;
  uint32_t thread_ids[ATOMS_MAX_THREADS_PER_PROC];
  uint32_t thread_count;
  uint32_t child_count;
  uint32_t module_ids[ATOMS_MAX_MODULES_PER_PROC];
  uint32_t module_count;
  uint32_t window_ids[ATOMS_MAX_WINDOWS_PER_PROC];
  uint32_t window_count;
  const char *environment[ATOMS_PROCESS_ENV_SLOTS];
  uint32_t environment_count;
  uint64_t capabilities_mask;
  uint64_t cpu_time_ms;
  uint64_t memory_used_bytes;
  int32_t exit_code;
  uint32_t ref_count;
  ATOMS_ProcessResourceUsage resources;
  ATOMS_ProcessDiagnostics diagnostics;
  uint64_t image_base;
  uint64_t image_end;
  uint64_t entry_point;
  uint64_t user_stack_guard;
  uint64_t exception_rip;
  uint64_t exception_address;
  uint64_t exception_error;
  uint32_t exception_vector;
  uint32_t user_faults;
  uint64_t pml4_phys;
  uint64_t heap_base;
  uint32_t heap_size;
  uint64_t stack_base;
  uint32_t stack_size;
} ATOMS_ProcessControlBlock;

typedef ATOMS_ProcessControlBlock ATOMS_PCB;

typedef struct {
  uint32_t capacity;
  uint32_t active;
  uint32_t zombies;
  uint32_t peak_active;
  uint32_t pid_min;
  uint32_t pid_max;
  uint64_t creations;
  uint64_t exits;
  uint64_t reaps;
  uint64_t invalid_transitions;
  uint64_t leaked_resources;
} ATOMS_ProcessManagerDiagnostics;

void ATOMS_ProcessManager_Init(void);
ATOMS_ExecutionErrorCode
ATOMS_ProcessManager_ConfigurePIDLimit(uint32_t maximum_pid);
ATOMS_PCB *ATOMS_Process_Create(const char *name, const char *filepath,
                                uint32_t parent_pid, uint32_t capabilities);
ATOMS_ExecutionErrorCode ATOMS_Process_Transition(ATOMS_PCB *pcb,
                                                  ATOMS_ProcessState state);
bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
ATOMS_ExecutionErrorCode ATOMS_Process_Reap(uint32_t pid,
                                            int32_t *out_exit_code);
ATOMS_PCB *ATOMS_Process_GetByPID(uint32_t pid);
ATOMS_PCB *ATOMS_Process_GetByIndex(uint32_t index);
uint32_t ATOMS_Process_GetCount(void);
uint32_t ATOMS_Process_EnumerateChildren(uint32_t parent_pid,
                                         uint32_t *out_pids, uint32_t capacity);
ATOMS_ExecutionErrorCode ATOMS_Process_RegisterThread(uint32_t pid,
                                                      uint32_t tid);
ATOMS_ExecutionErrorCode ATOMS_Process_UnregisterThread(uint32_t pid,
                                                        uint32_t tid);
void ATOMS_Process_AccountCPU(uint32_t pid, bool kernel_mode, uint64_t ticks);
void ATOMS_Process_GetManagerDiagnostics(ATOMS_ProcessManagerDiagnostics *out);
bool ATOMS_Process_AuditLeaks(uint64_t *out_leaked_resources);
void ATOMS_Process_DumpTelemetry(void);
uint32_t ATOMS_PID_Alloc(void);
void ATOMS_PID_Free(uint32_t pid);
int32_t ATOMS_Process_Wait(uint32_t pid, int32_t *out_exit_code);
ATOMS_ExecutionErrorCode
ATOMS_Process_SetUserImage(uint32_t pid, uint64_t pml4, uint64_t image_base,
                           uint64_t image_end, uint64_t entry_point,
                           uint64_t stack_base, uint64_t stack_size,
                           uint64_t guard_page);
void ATOMS_Process_RecordUserException(uint32_t pid, uint32_t vector,
                                       uint64_t rip, uint64_t address,
                                       uint64_t error_code);

#ifdef __cplusplus
}
#endif

#endif
