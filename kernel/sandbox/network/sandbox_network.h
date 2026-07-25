/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_network.h — Network Sandbox Subsystem Header
 */

#ifndef BOS_SANDBOX_NETWORK_H
#define BOS_SANDBOX_NETWORK_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_net_validate_connection(uint32_t context_id, uint32_t protocol, const char *address, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_NETWORK_H */
