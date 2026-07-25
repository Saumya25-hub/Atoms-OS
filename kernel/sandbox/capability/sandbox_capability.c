/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_capability.c — Capability Engine Subsystem Implementation
 */

#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t bos_capability_grant(uint32_t context_id, uint64_t capabilities) {
    return sandbox_capability_grant(context_id, capabilities);
}

bos_sandbox_status_t bos_capability_revoke(uint32_t context_id, uint64_t capabilities) {
    return sandbox_capability_revoke(context_id, capabilities);
}

bos_sandbox_status_t sandbox_capability_grant(uint32_t context_id, uint64_t capabilities) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    ctx->capabilities |= capabilities;
    bos_sandbox_debug_log(BOS_TRACE_CAPABILITY, "Granted capabilities 0x%llx to Context %u",
                          (unsigned long long)capabilities, context_id);

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_capability_revoke(uint32_t context_id, uint64_t capabilities) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    ctx->capabilities &= ~capabilities;
    bos_sandbox_debug_log(BOS_TRACE_CAPABILITY, "Revoked capabilities 0x%llx from Context %u",
                          (unsigned long long)capabilities, context_id);

    return BOS_SANDBOX_OK;
}

bool sandbox_capability_has(uint32_t context_id, uint64_t capability_flag) {
    bos_sandbox_context_t *ctx = NULL;
    if (sandbox_manager_get_context(context_id, &ctx) != BOS_SANDBOX_OK) {
        return false;
    }

    if (!ctx->active || ctx->crashed) return false;

    return (ctx->capabilities & capability_flag) == capability_flag;
}

bos_sandbox_status_t sandbox_capability_get_mask(uint32_t context_id, uint64_t *out_mask) {
    if (!out_mask) return BOS_SANDBOX_ERR_INVALID_PARAM;

    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    *out_mask = ctx->capabilities;
    return BOS_SANDBOX_OK;
}
