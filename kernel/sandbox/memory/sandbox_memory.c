/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_memory.c — Memory Isolation Subsystem Implementation
 */

#include "kernel/sandbox/memory/sandbox_memory.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

#define KERNEL_SPACE_START 0xFFFF800000000000ULL

bos_sandbox_status_t sandbox_memory_validate_access(uint32_t context_id, uint64_t vaddr, size_t len, bool is_write) {
    if (len == 0) return BOS_SANDBOX_OK;

    /* 1. Protected Kernel Memory Access Check */
    if (sandbox_memory_protect_kernel(vaddr, len) != BOS_SANDBOX_OK) {
        bos_audit_log_event(context_id, BOS_AUDIT_MEMORY_VIOLATION, "Attempted unauthorized access to kernel space memory", BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION);
        return BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION;
    }

    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    /* 2. Guard Page Check */
    if (sandbox_memory_guard_page_check(context_id, vaddr) != BOS_SANDBOX_OK ||
        sandbox_memory_guard_page_check(context_id, vaddr + len - 1) != BOS_SANDBOX_OK) {
        bos_audit_log_event(context_id, BOS_AUDIT_MEMORY_VIOLATION, "Guard page boundary violation detected", BOS_SANDBOX_ERR_GUARD_PAGE_VIOLATION);
        return BOS_SANDBOX_ERR_GUARD_PAGE_VIOLATION;
    }

    /* 3. Address Space Bounds Validation */
    if (ctx->virt_mem_start != 0 || ctx->virt_mem_end != 0) {
        if (vaddr < ctx->virt_mem_start || (vaddr + len) > ctx->virt_mem_end) {
            bos_audit_log_event(context_id, BOS_AUDIT_MEMORY_VIOLATION, "Address space boundary violation", BOS_SANDBOX_ERR_MEMORY_VIOLATION);
            return BOS_SANDBOX_ERR_MEMORY_VIOLATION;
        }
    }

    bos_sandbox_debug_log(BOS_TRACE_MEMORY, "Memory access validated for Context %u [0x%llx + %zu]", context_id, (unsigned long long)vaddr, len);
    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_memory_validate_cross_process(uint32_t src_context_id, uint32_t dst_context_id, uint64_t vaddr, size_t len) {
    if (src_context_id != dst_context_id) {
        bos_audit_log_event(src_context_id, BOS_AUDIT_MEMORY_VIOLATION, "Direct cross-process memory access blocked", BOS_SANDBOX_ERR_MEMORY_VIOLATION);
        return BOS_SANDBOX_ERR_MEMORY_VIOLATION;
    }

    return sandbox_memory_validate_access(src_context_id, vaddr, len, true);
}

bos_sandbox_status_t sandbox_memory_protect_kernel(uint64_t vaddr, size_t len) {
    if (vaddr >= KERNEL_SPACE_START || (vaddr + len) >= KERNEL_SPACE_START || vaddr < 0x10000ULL) {
        return BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION;
    }
    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_memory_guard_page_check(uint32_t context_id, uint64_t vaddr) {
    bos_sandbox_context_t *ctx = NULL;
    if (sandbox_manager_get_context(context_id, &ctx) != BOS_SANDBOX_OK) {
        return BOS_SANDBOX_OK; // Default pass if no context
    }

    if (ctx->virt_mem_start != 0) {
        /* Treat first and last page of region as Guard Pages */
        if (vaddr < (ctx->virt_mem_start + BOS_SANDBOX_GUARD_SIZE) ||
            vaddr >= (ctx->virt_mem_end - BOS_SANDBOX_GUARD_SIZE)) {
            return BOS_SANDBOX_ERR_GUARD_PAGE_VIOLATION;
        }
    }

    return BOS_SANDBOX_OK;
}
