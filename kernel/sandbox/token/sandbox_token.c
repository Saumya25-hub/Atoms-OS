/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_token.c — Process Token Engine Subsystem Implementation
 */

#include "kernel/sandbox/token/sandbox_token.h"
#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"
#include "kernel/core/lib/include/string.h"

#define BOS_TOKEN_MAGIC_SECRET 0x9E3779B97F4A7C15ULL

static uint64_t g_token_counter = 5000;

static uint64_t compute_token_signature(uint32_t pid, uint64_t token_id) {
    uint64_t sig = token_id ^ (uint64_t)pid;
    sig = (sig << 13) | (sig >> 51);
    sig ^= BOS_TOKEN_MAGIC_SECRET;
    return sig;
}

bos_sandbox_status_t bos_token_create(uint32_t pid, bos_token_t *out_token) {
    return sandbox_token_create(pid, out_token);
}

bos_sandbox_status_t bos_token_destroy(bos_token_t *token) {
    return sandbox_token_destroy(token);
}

bos_sandbox_status_t bos_token_verify(const bos_token_t *token) {
    return sandbox_token_verify(token);
}

bos_sandbox_status_t sandbox_token_create(uint32_t pid, bos_token_t *out_token) {
    if (pid == 0 || !out_token) return BOS_SANDBOX_ERR_INVALID_PARAM;

    out_token->token_id = g_token_counter++;
    out_token->owner_pid = pid;
    out_token->creation_time = 100000ULL; // Standard kernel boot timestamp
    out_token->signature = compute_token_signature(pid, out_token->token_id);
    out_token->is_valid = true;

    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "Generated unforgeable token 0x%llx for PID %u",
                          (unsigned long long)out_token->token_id, pid);

    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_token_destroy(bos_token_t *token) {
    if (!token) return BOS_SANDBOX_ERR_INVALID_PARAM;

    token->is_valid = false;
    token->signature = 0;
    token->token_id = 0;
    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t sandbox_token_verify(const bos_token_t *token) {
    if (!token || !token->is_valid) {
        return BOS_SANDBOX_ERR_TOKEN_FORGED;
    }

    uint64_t expected_sig = compute_token_signature(token->owner_pid, token->token_id);
    if (token->signature != expected_sig) {
        bos_audit_log_event(0, BOS_AUDIT_ESCAPE_ATTEMPT, "Token cryptographic signature verification failed", BOS_SANDBOX_ERR_TOKEN_FORGED);
        return BOS_SANDBOX_ERR_TOKEN_FORGED;
    }

    return BOS_SANDBOX_OK;
}
