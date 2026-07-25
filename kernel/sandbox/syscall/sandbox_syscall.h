/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_syscall.h — Syscall Filter Subsystem Header
 */

#ifndef BOS_SANDBOX_SYSCALL_H
#define BOS_SANDBOX_SYSCALL_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOS_SYS_READ              = 1,
    BOS_SYS_WRITE             = 2,
    BOS_SYS_OPEN              = 3,
    BOS_SYS_CLOSE             = 4,
    BOS_SYS_MMAP              = 5,
    BOS_SYS_MUNMAP            = 6,
    BOS_SYS_SOCKET_CONNECT    = 7,
    BOS_SYS_IPC_SEND          = 8,
    BOS_SYS_IPC_RECV          = 9,
    /* Denied / Privileged Syscalls */
    BOS_SYS_KERNEL_MODIFY     = 100,
    BOS_SYS_DRIVER_LOAD       = 101,
    BOS_SYS_RAW_MEM_IO        = 102,
    BOS_SYS_UNAUTHORIZED_DEV  = 103
} bos_syscall_id_t;

bos_sandbox_status_t sandbox_syscall_validate(uint32_t context_id, uint32_t syscall_num, const uint64_t *args, uint32_t arg_count);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_SYSCALL_H */
