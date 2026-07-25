/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_ipc.h — Secure IPC Policy Subsystem Header
 */

#ifndef BOS_SANDBOX_IPC_H
#define BOS_SANDBOX_IPC_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_ipc_validate_request(uint32_t sender_pid, uint32_t receiver_pid, uint32_t msg_type, size_t msg_size);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_IPC_H */
