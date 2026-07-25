/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_filesystem.h — Filesystem Sandbox Subsystem Header
 */

#ifndef BOS_SANDBOX_FILESYSTEM_H
#define BOS_SANDBOX_FILESYSTEM_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_fs_validate_path(uint32_t context_id, const char *path, uint32_t access_mode);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_FILESYSTEM_H */
