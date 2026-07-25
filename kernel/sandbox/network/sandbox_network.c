/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_network.c — Network Sandbox Subsystem Implementation
 */

#include "kernel/sandbox/network/sandbox_network.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t sandbox_net_validate_connection(uint32_t context_id, uint32_t protocol, const char *address, uint16_t port) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    /* Verify Network Capability */
    if (!sandbox_capability_has(context_id, BOS_CAP_NETWORK)) {
        bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_NETWORK capability", BOS_SANDBOX_ERR_NETWORK_DENIED);
        return BOS_SANDBOX_ERR_NETWORK_DENIED;
    }

    /* Reject RAW Sockets unless explicitly authorized by policy */
    if (protocol == 255 /* RAW socket */ && !ctx->allow_raw_sockets) {
        bos_audit_log_event(context_id, BOS_AUDIT_PERMISSION_DENIED, "Raw socket access rejected by network sandbox policy", BOS_SANDBOX_ERR_NETWORK_DENIED);
        return BOS_SANDBOX_ERR_NETWORK_DENIED;
    }

    /* Allow standard Web / Network Ports: HTTP (80), HTTPS (443), DNS (53), Custom ports (> 1024) */
    if (port == 80 || port == 443 || port == 53 || port >= 1024) {
        bos_sandbox_debug_log(BOS_TRACE_PERMISSION, "Network connection allowed for Context %u (Port %u)", context_id, port);
        return BOS_SANDBOX_OK;
    }

    /* Block restricted low ports */
    bos_audit_log_event(context_id, BOS_AUDIT_PERMISSION_DENIED, "Access denied to privileged low network port", BOS_SANDBOX_ERR_NETWORK_DENIED);
    return BOS_SANDBOX_ERR_NETWORK_DENIED;
}
