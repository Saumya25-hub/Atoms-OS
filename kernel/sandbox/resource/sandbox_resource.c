/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_resource.c — Resource Limit Engine Subsystem Implementation
 */

#include "kernel/sandbox/resource/sandbox_resource.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t bos_resource_limit(uint32_t context_id, const bos_resource_limits_t *limits) {
    if (!limits) return BOS_SANDBOX_ERR_INVALID_PARAM;

    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    ctx->limits = *limits;
    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Updated resource limits for Context %u", context_id);
    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_resource_track_alloc(uint32_t context_id, uint32_t resource_type, uint64_t amount) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    ctx->usage.total_allocations++;

    switch (resource_type) {
        case BOS_RES_RAM:
            if ((ctx->usage.ram_current_bytes + amount) > ctx->limits.ram_max_bytes) {
                bos_audit_log_event(context_id, BOS_AUDIT_RESOURCE_EXHAUSTION, "RAM resource limit exceeded", BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION);
                return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
            }
            ctx->usage.ram_current_bytes += amount;
            break;
        case BOS_RES_HANDLES:
            if ((ctx->usage.current_handles + (uint32_t)amount) > ctx->limits.max_handles) {
                bos_audit_log_event(context_id, BOS_AUDIT_RESOURCE_EXHAUSTION, "Handle allocation limit exceeded", BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION);
                return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
            }
            ctx->usage.current_handles += (uint32_t)amount;
            break;
        case BOS_RES_THREADS:
            if ((ctx->usage.current_threads + (uint32_t)amount) > ctx->limits.max_threads) {
                bos_audit_log_event(context_id, BOS_AUDIT_RESOURCE_EXHAUSTION, "Thread limit exceeded", BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION);
                return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
            }
            ctx->usage.current_threads += (uint32_t)amount;
            break;
        default:
            break;
    }

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_resource_track_free(uint32_t context_id, uint32_t resource_type, uint64_t amount) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    ctx->usage.total_frees++;

    switch (resource_type) {
        case BOS_RES_RAM:
            if (ctx->usage.ram_current_bytes >= amount) {
                ctx->usage.ram_current_bytes -= amount;
            } else {
                ctx->usage.ram_current_bytes = 0;
            }
            break;
        case BOS_RES_HANDLES:
            if (ctx->usage.current_handles >= (uint32_t)amount) {
                ctx->usage.current_handles -= (uint32_t)amount;
            } else {
                ctx->usage.current_handles = 0;
            }
            break;
        default:
            break;
    }

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_resource_check_limits(uint32_t context_id, uint32_t resource_type) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    switch (resource_type) {
        case BOS_RES_RAM:
            if (ctx->usage.ram_current_bytes >= ctx->limits.ram_max_bytes) return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
            break;
        case BOS_RES_HANDLES:
            if (ctx->usage.current_handles >= ctx->limits.max_handles) return BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION;
            break;
        default:
            break;
    }

    return BOS_SANDBOX_OK;
}

bool sandbox_resource_detect_leaks(uint32_t context_id) {
    bos_sandbox_context_t *ctx = NULL;
    if (sandbox_manager_get_context(context_id, &ctx) != BOS_SANDBOX_OK) {
        return false;
    }

    /* Check if allocations exceeded frees without context cleanup */
    if (ctx->usage.total_allocations > ctx->usage.total_frees && ctx->usage.current_handles > 0) {
        bos_audit_log_event(context_id, BOS_AUDIT_RESOURCE_EXHAUSTION, "Resource handle leak detected by resource audit engine", BOS_SANDBOX_ERR_RESOURCE_EXHAUSTION);
        return true;
    }

    return false;
}
