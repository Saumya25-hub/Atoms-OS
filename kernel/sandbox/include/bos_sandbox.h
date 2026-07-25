/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * bos_sandbox.h — Master Subsystem Public Facade API
 */

#ifndef BOS_SANDBOX_H
#define BOS_SANDBOX_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the complete BOS OS Sandbox Engine.
 * @return BOS_SANDBOX_OK on success or structured error code.
 */
bos_sandbox_status_t bos_sandbox_init(void);

/**
 * @brief Checks if the sandbox subsystem is initialized.
 * @return true if initialized, false otherwise.
 */
bool bos_sandbox_is_initialized(void);

/**
 * @brief Creates a new process sandbox context.
 */
bos_sandbox_status_t bos_sandbox_create(uint32_t pid,
                                       uint64_t initial_capabilities,
                                       const bos_resource_limits_t *limits,
                                       uint32_t *out_context_id);

/**
 * @brief Destroys an existing process sandbox context and releases all isolated resources.
 */
bos_sandbox_status_t bos_sandbox_destroy(uint32_t context_id);

/**
 * @brief Grants fine-grained capabilities to a process sandbox context.
 */
bos_sandbox_status_t bos_capability_grant(uint32_t context_id, uint64_t capabilities);

/**
 * @brief Revokes capabilities from a process sandbox context.
 */
bos_sandbox_status_t bos_capability_revoke(uint32_t context_id, uint64_t capabilities);

/**
 * @brief Validates a central permission request for a sensitive operation.
 */
bos_sandbox_status_t bos_permission_check(uint32_t context_id, uint32_t perm_op, const void *op_data);

/**
 * @brief Generates an unforgeable security token for a process.
 */
bos_sandbox_status_t bos_token_create(uint32_t pid, bos_token_t *out_token);

/**
 * @brief Destroys and invalidates a process security token.
 */
bos_sandbox_status_t bos_token_destroy(bos_token_t *token);

/**
 * @brief Validates a system call invocation against context policy and capabilities.
 */
bos_sandbox_status_t bos_syscall_validate(uint32_t context_id, uint32_t syscall_num, const uint64_t *args, uint32_t arg_count);

/**
 * @brief Configures resource limits for an isolated process sandbox.
 */
bos_sandbox_status_t bos_resource_limit(uint32_t context_id, const bos_resource_limits_t *limits);

/**
 * @brief Isolates a process into its own secure execution boundary.
 */
bos_sandbox_status_t bos_process_isolate(uint32_t pid, uint64_t page_table_base, uint64_t mem_start, uint64_t mem_size, uint32_t *out_context_id);

/**
 * @brief Executes the complete certification test suite for Phase 4 Sandbox.
 */
bos_sandbox_status_t bos_sandbox_run_certification_tests(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_H */
