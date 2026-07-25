/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_syscall.c — Syscall Filter Subsystem Implementation
 */

#include "kernel/sandbox/syscall/sandbox_syscall.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"

bos_sandbox_status_t bos_syscall_validate(uint32_t context_id, uint32_t syscall_num, const uint64_t *args, uint32_t arg_count) {
    return sandbox_syscall_validate(context_id, syscall_num, args, arg_count);
}

bos_sandbox_status_t sandbox_syscall_validate(uint32_t context_id, uint32_t syscall_num, const uint64_t *args, uint32_t arg_count) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    if (!ctx->active || ctx->crashed) {
        return BOS_SANDBOX_ERR_SYSCALL_BLOCKED;
    }

    /* Deny privileged / unsafe syscalls */
    if (syscall_num >= 100) {
        bos_audit_log_event(context_id, BOS_AUDIT_INVALID_SYSCALL, "Blocked privileged syscall invocation", BOS_SANDBOX_ERR_SYSCALL_BLOCKED);
        bos_sandbox_debug_log(BOS_TRACE_SYSCALL, "Syscall %u blocked for Context %u", syscall_num, context_id);
        return BOS_SANDBOX_ERR_SYSCALL_BLOCKED;
    }

    /* Allowed standard syscalls */
    switch (syscall_num) {
        case BOS_SYS_READ:
        case BOS_SYS_WRITE:
        case BOS_SYS_OPEN:
        case BOS_SYS_CLOSE:
        case BOS_SYS_MMAP:
        case BOS_SYS_MUNMAP:
        case BOS_SYS_SOCKET_CONNECT:
        case BOS_SYS_IPC_SEND:
        case BOS_SYS_IPC_RECV:
            break;
        default:
            bos_audit_log_event(context_id, BOS_AUDIT_INVALID_SYSCALL, "Unknown / unauthorized syscall number", BOS_SANDBOX_ERR_SYSCALL_BLOCKED);
            return BOS_SANDBOX_ERR_SYSCALL_BLOCKED;
    }

    bos_sandbox_debug_log(BOS_TRACE_SYSCALL, "Syscall %u validated for Context %u", syscall_num, context_id);
    return BOS_SANDBOX_OK;
}
