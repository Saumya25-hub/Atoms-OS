/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_resource.h — Resource Limit Engine Subsystem Header
 */

#ifndef BOS_SANDBOX_RESOURCE_H
#define BOS_SANDBOX_RESOURCE_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOS_RES_RAM          = 1,
    BOS_RES_THREADS      = 2,
    BOS_RES_HANDLES      = 3,
    BOS_RES_IPC_OBJECTS  = 4,
    BOS_RES_SHM_BYTES    = 5,
    BOS_RES_OPEN_FILES   = 6,
    BOS_RES_NET_CONNS    = 7
} bos_resource_type_t;

bos_sandbox_status_t bos_resource_limit(uint32_t context_id, const bos_resource_limits_t *limits);
bos_sandbox_status_t sandbox_resource_track_alloc(uint32_t context_id, uint32_t resource_type, uint64_t amount);
bos_sandbox_status_t sandbox_resource_track_free(uint32_t context_id, uint32_t resource_type, uint64_t amount);
bos_sandbox_status_t sandbox_resource_check_limits(uint32_t context_id, uint32_t resource_type);
bool                 sandbox_resource_detect_leaks(uint32_t context_id);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_RESOURCE_H */
