/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_filesystem.c — Filesystem Sandbox Subsystem Implementation
 */

#include "kernel/sandbox/filesystem/sandbox_filesystem.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"
#include "kernel/core/lib/include/string.h"

bos_sandbox_status_t sandbox_fs_validate_path(uint32_t context_id, const char *path, uint32_t access_mode) {
    if (!path) return BOS_SANDBOX_ERR_INVALID_PARAM;

    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    /* Check required capabilities */
    if (access_mode == 1) { // Read
        if (!sandbox_capability_has(context_id, BOS_CAP_READ_FILES)) {
            bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_READ_FILES capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
            return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
        }
    } else if (access_mode == 2) { // Write/Delete
        if (!sandbox_capability_has(context_id, BOS_CAP_WRITE_FILES) && !sandbox_capability_has(context_id, BOS_CAP_DELETE_FILES)) {
            bos_audit_log_event(context_id, BOS_AUDIT_CAPABILITY_VIOLATION, "Missing BOS_CAP_WRITE_FILES capability", BOS_SANDBOX_ERR_CAPABILITY_DENIED);
            return BOS_SANDBOX_ERR_CAPABILITY_DENIED;
        }
    }

    /* Deny forbidden system directories */
    if (strncmp(path, "/kernel/", 8) == 0 ||
        strncmp(path, "/drivers/", 9) == 0 ||
        strncmp(path, "/sys/", 5) == 0 ||
        strncmp(path, "/security/", 10) == 0) {
        bos_audit_log_event(context_id, BOS_AUDIT_PERMISSION_DENIED, "Access denied to protected OS system directory", BOS_SANDBOX_ERR_PATH_DENIED);
        bos_sandbox_debug_log(BOS_TRACE_PERMISSION, "Filesystem path '%s' denied for Context %u", path, context_id);
        return BOS_SANDBOX_ERR_PATH_DENIED;
    }

    /* Allow standard sandboxed directories: /cache/, /downloads/, /tmp/, /app_data/, /user/ */
    if (strncmp(path, "/cache/", 7) == 0 ||
        strncmp(path, "/downloads/", 11) == 0 ||
        strncmp(path, "/tmp/", 5) == 0 ||
        strncmp(path, "/user/", 6) == 0 ||
        (ctx->allowed_fs_prefix[0] != '\0' && strncmp(path, ctx->allowed_fs_prefix, strlen(ctx->allowed_fs_prefix)) == 0)) {
        bos_sandbox_debug_log(BOS_TRACE_PERMISSION, "Filesystem path '%s' allowed for Context %u", path, context_id);
        return BOS_SANDBOX_OK;
    }

    /* Block outside path access */
    bos_audit_log_event(context_id, BOS_AUDIT_PERMISSION_DENIED, "Filesystem path outside sandbox boundary", BOS_SANDBOX_ERR_PATH_DENIED);
    return BOS_SANDBOX_ERR_PATH_DENIED;
}
