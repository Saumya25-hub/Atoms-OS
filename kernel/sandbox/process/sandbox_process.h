/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_process.h — Process Isolation Subsystem Header
 */

#ifndef BOS_SANDBOX_PROCESS_H
#define BOS_SANDBOX_PROCESS_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t bos_process_isolate(uint32_t pid,
                                       uint64_t page_table_base,
                                       uint64_t mem_start,
                                       uint64_t mem_size,
                                       uint32_t *out_context_id);

bos_sandbox_status_t sandbox_process_isolate(uint32_t pid,
                                              uint64_t page_table_base,
                                              uint64_t mem_start,
                                              uint64_t mem_size,
                                              uint32_t *out_context_id);

bool sandbox_process_is_isolated(uint32_t pid);
bos_sandbox_status_t sandbox_process_handle_crash(uint32_t pid);
bos_sandbox_status_t sandbox_process_get_memory_bounds(uint32_t pid, uint64_t *out_start, uint64_t *out_end);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_PROCESS_H */
