/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_process.c — Process Isolation Subsystem Implementation
 */

#include "kernel/sandbox/process/sandbox_process.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"
#include "kernel/core/lib/include/string.h"

bos_sandbox_status_t bos_process_isolate(uint32_t pid,
                                       uint64_t page_table_base,
                                       uint64_t mem_start,
                                       uint64_t mem_size,
                                       uint32_t *out_context_id) {
    return sandbox_process_isolate(pid, page_table_base, mem_start, mem_size, out_context_id);
}

bos_sandbox_status_t sandbox_process_isolate(uint32_t pid,
                                              uint64_t page_table_base,
                                              uint64_t mem_start,
                                              uint64_t mem_size,
                                              uint32_t *out_context_id) {
    if (pid == 0 || mem_size == 0 || !out_context_id) {
        return BOS_SANDBOX_ERR_INVALID_PARAM;
    }

    /* Check if process is already isolated */
    bos_sandbox_context_t *existing_ctx = NULL;
    if (sandbox_manager_find_by_pid(pid, &existing_ctx) == BOS_SANDBOX_OK) {
        *out_context_id = existing_ctx->context_id;
        return BOS_SANDBOX_OK;
    }

    uint32_t context_id = 0;
    uint64_t default_caps = BOS_CAP_READ_FILES | BOS_CAP_TEMP_STORAGE | BOS_CAP_NOTIFICATIONS;
    bos_sandbox_status_t status = sandbox_manager_create_context(pid, default_caps, NULL, &context_id);
    if (status != BOS_SANDBOX_OK) {
        return status;
    }

    bos_sandbox_context_t *ctx = NULL;
    status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) {
        return status;
    }

    ctx->page_table_base = page_table_base;
    ctx->virt_mem_start  = mem_start;
    ctx->virt_mem_end    = mem_start + mem_size;

    *out_context_id = context_id;
    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Isolated process PID %u into Context %u [Mem 0x%llx - 0x%llx]",
                          pid, context_id, (unsigned long long)mem_start, (unsigned long long)(mem_start + mem_size));

    return BOS_SANDBOX_OK;
}

bool sandbox_process_is_isolated(uint32_t pid) {
    bos_sandbox_context_t *ctx = NULL;
    if (sandbox_manager_find_by_pid(pid, &ctx) == BOS_SANDBOX_OK) {
        return ctx->active && !ctx->crashed;
    }
    return false;
}

bos_sandbox_status_t sandbox_process_handle_crash(uint32_t pid) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_find_by_pid(pid, &ctx);
    if (status != BOS_SANDBOX_OK) {
        return status;
    }

    /* Log process crash event to audit trail */
    bos_audit_log_event(ctx->context_id, BOS_AUDIT_PROCESS_CRASH, "Isolated process crashed; reclaiming sandbox resources", BOS_SANDBOX_ERR_CRASH_ISOLATED);

    /* Mark context as crashed and destroy it safely without affecting kernel or other apps */
    ctx->crashed = true;
    uint32_t cid = ctx->context_id;
    sandbox_manager_destroy_context(cid);

    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Crash isolation completed for PID %u (Context %u destroyed, OS preserved)", pid, cid);

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_process_get_memory_bounds(uint32_t pid, uint64_t *out_start, uint64_t *out_end) {
    if (!out_start || !out_end) return BOS_SANDBOX_ERR_INVALID_PARAM;

    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_find_by_pid(pid, &ctx);
    if (status != BOS_SANDBOX_OK) {
        return status;
    }

    *out_start = ctx->virt_mem_start;
    *out_end   = ctx->virt_mem_end;
    return BOS_SANDBOX_OK;
}
