/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_manager.c — Master Sandbox Manager Subsystem Implementation
 */

#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/token/sandbox_token.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_sandbox_initialized = false;
static bos_sandbox_context_t g_contexts[BOS_MAX_SANDBOX_CONTEXTS];
static uint32_t g_next_context_id = 1000;

bos_sandbox_status_t bos_sandbox_init(void) {
    if (g_sandbox_initialized) {
        return BOS_SANDBOX_ERR_ALREADY_INITIALIZED;
    }

    display_print("==============================================\n");
    display_print("[SANDBOX]\n");
    display_print("Initializing BOS Sandbox...\n");
    display_print("Capability Engine Ready\n");
    display_print("Permission Manager Ready\n");
    display_print("Token Manager Ready\n");
    display_print("Memory Isolation Ready\n");
    display_print("Syscall Filter Ready\n");
    display_print("Filesystem Sandbox Ready\n");
    display_print("Network Sandbox Ready\n");
    display_print("Resource Manager Ready\n");
    display_print("==============================================\n\n");

    sandbox_debug_init();
    sandbox_audit_init();

    memset(g_contexts, 0, sizeof(g_contexts));
    g_next_context_id = 1000;
    g_sandbox_initialized = true;

    return BOS_SANDBOX_OK;
}

bool bos_sandbox_is_initialized(void) {
    return g_sandbox_initialized;
}

bos_sandbox_status_t sandbox_manager_init(void) {
    return bos_sandbox_init();
}

bool sandbox_manager_is_initialized(void) {
    return g_sandbox_initialized;
}

bos_sandbox_status_t bos_sandbox_create(uint32_t pid,
                                       uint64_t initial_capabilities,
                                       const bos_resource_limits_t *limits,
                                       uint32_t *out_context_id) {
    return sandbox_manager_create_context(pid, initial_capabilities, limits, out_context_id);
}

bos_sandbox_status_t bos_sandbox_destroy(uint32_t context_id) {
    return sandbox_manager_destroy_context(context_id);
}

bos_sandbox_status_t sandbox_manager_create_context(uint32_t pid,
                                                    uint64_t capabilities,
                                                    const bos_resource_limits_t *limits,
                                                    uint32_t *out_context_id) {
    if (!g_sandbox_initialized) return BOS_SANDBOX_ERR_NOT_INITIALIZED;
    if (pid == 0 || !out_context_id) return BOS_SANDBOX_ERR_INVALID_PARAM;

    int slot = -1;
    for (int i = 0; i < BOS_MAX_SANDBOX_CONTEXTS; i++) {
        if (!g_contexts[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        bos_audit_log_event(0, BOS_AUDIT_RESOURCE_EXHAUSTION, "Max sandbox context slot limit reached", BOS_SANDBOX_ERR_NO_MEMORY);
        return BOS_SANDBOX_ERR_NO_MEMORY;
    }

    bos_sandbox_context_t *ctx = &g_contexts[slot];
    memset(ctx, 0, sizeof(bos_sandbox_context_t));

    ctx->context_id = g_next_context_id++;
    ctx->pid = pid;
    ctx->capabilities = capabilities;
    ctx->active = true;
    ctx->crashed = false;

    /* Initialize Token */
    bos_sandbox_status_t token_res = bos_token_create(pid, &ctx->token);
    if (token_res != BOS_SANDBOX_OK) {
        ctx->active = false;
        return token_res;
    }

    /* Assign Resource Limits */
    if (limits) {
        ctx->limits = *limits;
    } else {
        /* Default baseline sandbox resource limits */
        ctx->limits.cpu_max_percent = 80;
        ctx->limits.ram_max_bytes = 256 * 1024 * 1024ULL; // 256 MB default
        ctx->limits.max_threads = 32;
        ctx->limits.max_handles = 128;
        ctx->limits.max_ipc_objects = 64;
        ctx->limits.max_shm_bytes = 16 * 1024 * 1024ULL;
        ctx->limits.max_open_files = 64;
        ctx->limits.max_net_conns = 32;
    }

    /* Assign Default Filesystem Sandbox Policy */
    strncpy(ctx->allowed_fs_prefix, "/app_data/", BOS_MAX_PATH_LENGTH - 1);

    *out_context_id = ctx->context_id;
    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Created sandbox context %u for PID %u", ctx->context_id, pid);

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_manager_destroy_context(uint32_t context_id) {
    if (!g_sandbox_initialized) return BOS_SANDBOX_ERR_NOT_INITIALIZED;
    if (context_id == 0) return BOS_SANDBOX_ERR_INVALID_PARAM;

    for (int i = 0; i < BOS_MAX_SANDBOX_CONTEXTS; i++) {
        if (g_contexts[i].active && g_contexts[i].context_id == context_id) {
            bos_token_destroy(&g_contexts[i].token);
            g_contexts[i].active = false;
            g_contexts[i].crashed = false;
            memset(&g_contexts[i], 0, sizeof(bos_sandbox_context_t));
            bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Destroyed sandbox context %u", context_id);
            return BOS_SANDBOX_OK;
        }
    }

    return BOS_SANDBOX_ERR_CONTEXT_NOT_FOUND;
}

bos_sandbox_status_t sandbox_manager_get_context(uint32_t context_id, bos_sandbox_context_t **out_context) {
    if (!g_sandbox_initialized) return BOS_SANDBOX_ERR_NOT_INITIALIZED;
    if (context_id == 0 || !out_context) return BOS_SANDBOX_ERR_INVALID_PARAM;

    for (int i = 0; i < BOS_MAX_SANDBOX_CONTEXTS; i++) {
        if (g_contexts[i].active && g_contexts[i].context_id == context_id) {
            *out_context = &g_contexts[i];
            return BOS_SANDBOX_OK;
        }
    }

    return BOS_SANDBOX_ERR_CONTEXT_NOT_FOUND;
}

bos_sandbox_status_t sandbox_manager_find_by_pid(uint32_t pid, bos_sandbox_context_t **out_context) {
    if (!g_sandbox_initialized) return BOS_SANDBOX_ERR_NOT_INITIALIZED;
    if (pid == 0 || !out_context) return BOS_SANDBOX_ERR_INVALID_PARAM;

    for (int i = 0; i < BOS_MAX_SANDBOX_CONTEXTS; i++) {
        if (g_contexts[i].active && g_contexts[i].pid == pid) {
            *out_context = &g_contexts[i];
            return BOS_SANDBOX_OK;
        }
    }

    return BOS_SANDBOX_ERR_CONTEXT_NOT_FOUND;
}

bos_sandbox_status_t sandbox_manager_validate_privileged_request(uint32_t context_id, uint32_t req_type, const void *req_data) {
    bos_sandbox_context_t *ctx = NULL;
    bos_sandbox_status_t status = sandbox_manager_get_context(context_id, &ctx);
    if (status != BOS_SANDBOX_OK) return status;

    if (!ctx->active || ctx->crashed) {
        return BOS_SANDBOX_ERR_PERMISSION_DENIED;
    }

    /* Verify Token Integrity */
    if (bos_token_verify(&ctx->token) != BOS_SANDBOX_OK) {
        bos_audit_log_event(context_id, BOS_AUDIT_ESCAPE_ATTEMPT, "Token forgery detected during privileged validation", BOS_SANDBOX_ERR_TOKEN_FORGED);
        return BOS_SANDBOX_ERR_TOKEN_FORGED;
    }

    return BOS_SANDBOX_OK;
}
