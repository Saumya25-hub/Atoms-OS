/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_debug.h — Debug Engine Subsystem Header
 */

#ifndef BOS_SANDBOX_DEBUG_H
#define BOS_SANDBOX_DEBUG_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void sandbox_debug_init(void);
void bos_sandbox_debug_set_trace(uint64_t trace_mask, bool enable);
uint64_t bos_sandbox_debug_get_trace_mask(void);
void bos_sandbox_debug_log(uint64_t trace_flag, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_DEBUG_H */
