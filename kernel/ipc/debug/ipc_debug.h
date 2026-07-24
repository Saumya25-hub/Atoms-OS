/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_debug.h — Debug & Trace Logging Framework
 */

#ifndef BOS_IPC_DEBUG_H
#define BOS_IPC_DEBUG_H

#include <stdint.h>

typedef enum {
    IPC_LOG_NONE  = 0,
    IPC_LOG_ERROR = 1,
    IPC_LOG_WARN  = 2,
    IPC_LOG_INFO  = 3,
    IPC_LOG_TRACE = 4
} ipc_log_level_t;

void ipc_debug_init(void);
void ipc_debug_set_level(ipc_log_level_t level);
void ipc_debug_log(ipc_log_level_t level, const char* subsystem, const char* message);
void ipc_debug_log_hex(ipc_log_level_t level, const char* subsystem, const char* prefix, uint64_t value);

#endif /* BOS_IPC_DEBUG_H */
