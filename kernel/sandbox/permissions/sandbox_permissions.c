/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_permissions.c — Permission Manager Subsystem Implementation
 */

#include "kernel/sandbox/permissions/sandbox_permissions.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/filesystem/sandbox_filesystem.h"
#include "kernel/sandbox/network/sandbox_network.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t bos_permission_check(uint32_t context_id, uint32_t perm_op, const void *op_data) {
    return sandbox_permissions_check(context_id, perm_op, op_data);
}

bos_sandbox_status_t sandbox_permissions_check(uint32_t context_id, uint32_t perm_op, const void *op_data) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    if (!ctx->active || ctx->crashed) {
        return BOS_SANDBOX_ERR_PERMISSION_DENIED;
    }

    uint64_t req_cap = BOS_CAP_NONE;

    switch (perm_op) {
        case BOS_PERM_OPEN_FILE:
            req_cap = BOS_CAP_READ_FILES;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_READ_FILES capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            if (op_data) {
                return sandbox_fs_validate_path(context_id, (const char*)op_data, 1);
            }
            break;

        case BOS_PERM_DELETE_FILE:
            req_cap = BOS_CAP_DELETE_FILES;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_DELETE_FILES capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            if (op_data) {
                return sandbox_fs_validate_path(context_id, (const char*)op_data, 2);
            }
            break;

        case BOS_PERM_CONNECT_NETWORK:
            req_cap = BOS_CAP_NETWORK;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_NETWORK capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            break;

        case BOS_PERM_ACCESS_CLIPBOARD:
            req_cap = BOS_CAP_CLIPBOARD;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_CLIPBOARD capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            break;

        case BOS_PERM_OPEN_CAMERA:
            req_cap = BOS_CAP_CAMERA;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_CAMERA capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            break;

        case BOS_PERM_ACCESS_MICROPHONE:
            req_cap = BOS_CAP_MICROPHONE;
            if (!sandbox_capability_has(context_id, req_cap)) {
                bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_MICROPHONE capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
                return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
            }
            break;

        case BOS_PERM_CREATE_SHM:
        case BOS_PERM_CREATE_PIPE:
        case BOS_PERM_MAP_MEMORY:
        case BOS_PERM_SPAWN_PROCESS:
            /* Validated by IPC / Resource limits */
            break;

        default:
            return BOS_SANDBOX_ERR_INVALID_PARAM;
    }

    bos_sandbox_debug_log(BOS_TRACE_PERMISSION, "Permission check passed for Context %u, Op %u", context_id, perm_op);
    return BOS_SANDBOX_OK;
}
