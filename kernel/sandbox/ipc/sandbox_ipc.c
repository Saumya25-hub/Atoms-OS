/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_ipc.c — Secure IPC Policy Subsystem Implementation
 */

#include "kernel/sandbox/ipc/sandbox_ipc.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/token/sandbox_token.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t sandbox_ipc_validate_request(uint32_t sender_pid, uint32_t receiver_pid, uint32_t msg_type, size_t msg_size) {
    if (sender_pid == 0 || receiver_pid == 0) {
        return BOS_SANDBOX_ERR_INVALID_PARAM;
    }

    bos_sandbox_context_t *src_ctx = NULL;
    bos_sandbox_context_t *dst_ctx = NULL;

    bos_sandbox_status_t status1 = sandbox_manager_find_by_pid(sender_pid, &src_ctx);
    bos_sandbox_status_t status2 = sandbox_manager_find_by_pid(receiver_pid, &dst_ctx);

    if (status1 != BOS_SANDBOX_OK || status2 != BOS_SANDBOX_OK) {
        bos_audit_log_event(0, BOS_AUDIT_UNAUTHORIZED_IPC, "IPC between un-isolated/invalid processes", BOS_SANDBOX_ERR_IPC_DENIED);
        return BOS_SANDBOX_ERR_IPC_DENIED;
    }

    /* Verify sender & receiver tokens */
    if (bos_token_verify(&src_ctx->token) != BOS_SANDBOX_OK ||
        bos_token_verify(&dst_ctx->token) != BOS_SANDBOX_OK) {
        bos_audit_log_event(src_ctx->context_id, BOS_AUDIT_UNAUTHORIZED_IPC, "Token forgery in IPC request", BOS_SANDBOX_ERR_TOKEN_FORGED);
        return BOS_SANDBOX_ERR_TOKEN_FORGED;
    }

    /* Verify receiver context is active */
    if (!dst_ctx->active || dst_ctx->crashed) {
        return BOS_SANDBOX_ERR_IPC_DENIED;
    }

    /* Verify max message size limit */
    if (msg_size > (64 * 1024)) { // 64KB IPC message cap
        bos_audit_log_event(src_ctx->context_id, BOS_AUDIT_RESOURCE_EXHAUSTION, "IPC message size exceeds limit", BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION);
        return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
    }

    bos_sandbox_debug_log(BOS_TRACE_IPC, "IPC validated from PID %u to PID %u (size %zu)", sender_pid, receiver_pid, msg_size);
    return BOS_SANDBOX_OK;
}
