/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_permissions.h — Permission Manager Subsystem Header
 */

#ifndef BOS_SANDBOX_PERMISSIONS_H
#define BOS_SANDBOX_PERMISSIONS_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_permissions_check(uint32_t context_id, uint32_t perm_op, const void *op_data);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_PERMISSIONS_H */
