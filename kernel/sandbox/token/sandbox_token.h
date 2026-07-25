/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_token.h — Process Token Engine Subsystem Header
 */

#ifndef BOS_SANDBOX_TOKEN_H
#define BOS_SANDBOX_TOKEN_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t bos_token_create(uint32_t pid, bos_token_t *out_token);
bos_sandbox_status_t bos_token_destroy(bos_token_t *token);
bos_sandbox_status_t bos_token_verify(const bos_token_t *token);

bos_sandbox_status_t sandbox_token_create(uint32_t pid, bos_token_t *out_token);
bos_sandbox_status_t sandbox_token_destroy(bos_token_t *token);
bos_sandbox_status_t sandbox_token_verify(const bos_token_t *token);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_TOKEN_H */
