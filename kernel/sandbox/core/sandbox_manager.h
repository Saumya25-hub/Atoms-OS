/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_manager.h — Master Sandbox Manager Subsystem Header
 */

#ifndef BOS_SANDBOX_MANAGER_H
#define BOS_SANDBOX_MANAGER_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_manager_init(void);
bool sandbox_manager_is_initialized(void);

bos_sandbox_status_t sandbox_manager_create_context(uint32_t pid,
                                                    uint64_t capabilities,
                                                    const bos_resource_limits_t *limits,
                                                    uint32_t *out_context_id);

bos_sandbox_status_t sandbox_manager_destroy_context(uint32_t context_id);
bos_sandbox_status_t sandbox_manager_get_context(uint32_t context_id, bos_sandbox_context_t **out_context);
bos_sandbox_status_t sandbox_manager_find_by_pid(uint32_t pid, bos_sandbox_context_t **out_context);

bos_sandbox_status_t sandbox_manager_validate_privileged_request(uint32_t context_id, uint32_t req_type, const void *req_data);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_MANAGER_H */
