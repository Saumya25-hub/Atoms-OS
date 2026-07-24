/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_manager.h — Subsystem Lifecycle Orchestrator
 */

#ifndef BOS_IPC_MANAGER_H
#define BOS_IPC_MANAGER_H

#include "kernel/ipc/include/ipc_types.h"

ipc_status_t bos_ipc_init(void);
ipc_status_t bos_ipc_shutdown(void);

#endif /* BOS_IPC_MANAGER_H */
