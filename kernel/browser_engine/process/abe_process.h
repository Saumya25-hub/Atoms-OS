#ifndef ABE_PROCESS_H
#define ABE_PROCESS_H

#include "../../../sdk/include/abe/abe.h"

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
    uint32_t process_id;
    ABE_ProcessRole role;
    ABE_ProcessState state;
    uint32_t owner_pid;
    size_t memory_allocated_bytes;
    uint64_t start_timestamp;
    bool crash_safe_cleanup;
} ABE_ProcessNode;

typedef struct {
    ABE_ProcessNode processes[ABE_MAX_PROCESSES];
    uint32_t process_count;
    uint32_t main_browser_pid;
    bool is_active;
} ABE_ProcessManager;

ABE_Error ABE_Process_Init(void);
ABE_Error ABE_Process_Shutdown(void);

ABE_Error ABE_Process_Create(ABE_ProcessRole role, uint32_t* out_pid);
ABE_Error ABE_Process_Terminate(uint32_t pid);
ABE_Error ABE_Process_CrashHandler(uint32_t pid);

#ifdef __cplusplus
}
#endif

#endif // ABE_PROCESS_H
