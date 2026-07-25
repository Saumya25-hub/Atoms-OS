/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_memory.h — Memory Isolation Subsystem Header
 */

#ifndef BOS_SANDBOX_MEMORY_H
#define BOS_SANDBOX_MEMORY_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_memory_validate_access(uint32_t context_id, uint64_t vaddr, size_t len, bool is_write);
bos_sandbox_status_t sandbox_memory_validate_cross_process(uint32_t src_context_id, uint32_t dst_context_id, uint64_t vaddr, size_t len);
bos_sandbox_status_t sandbox_memory_protect_kernel(uint64_t vaddr, size_t len);
bos_sandbox_status_t sandbox_memory_guard_page_check(uint32_t context_id, uint64_t vaddr);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_MEMORY_H */
