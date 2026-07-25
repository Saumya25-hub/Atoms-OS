/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_capability.h — Capability Engine Subsystem Header
 */

#ifndef BOS_SANDBOX_CAPABILITY_H
#define BOS_SANDBOX_CAPABILITY_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_capability_grant(uint32_t context_id, uint64_t capabilities);
bos_sandbox_status_t sandbox_capability_revoke(uint32_t context_id, uint64_t capabilities);
bool                 sandbox_capability_has(uint32_t context_id, uint64_t capability_flag);
bos_sandbox_status_t sandbox_capability_get_mask(uint32_t context_id, uint64_t *out_mask);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_CAPABILITY_H */
