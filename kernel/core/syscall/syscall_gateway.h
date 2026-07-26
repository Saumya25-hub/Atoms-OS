#ifndef ATOMS_SYSCALL_GATEWAY_H
#define ATOMS_SYSCALL_GATEWAY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// Kernel-internal application service gateway (Phase 10 legacy).
// This is NOT the Ring-3 SYSCALL ABI and must not be called with raw user
// pointers. The sole production Ring-3 authority is syscall_handler().
// ============================================================

#define SYS_WINDOW_CREATE 1
#define SYS_WINDOW_CLOSE 2
#define SYS_FILESYSTEM_OPEN 3
#define SYS_FILESYSTEM_READ 4
#define SYS_FILESYSTEM_WRITE 5
#define SYS_CLIPBOARD_SET 6
#define SYS_CLIPBOARD_GET 7
#define SYS_DIALOG_SHOW 8
#define SYS_MEMORY_ALLOC 9
#define SYS_THREAD_CREATE 10
#define SYS_TIMER_CREATE 11
#define SYS_AUDIO_PLAY 12

void ATOMS_SyscallGateway_Init(void);
uint64_t ATOMS_KernelService_Dispatch(uint32_t service_nr, uint64_t arg1,
                                      uint64_t arg2, uint64_t arg3,
                                      uint64_t arg4);
/* Source compatibility only; remains kernel-internal. */
static inline uint64_t ATOMS_Syscall_Dispatch(uint32_t service_nr,
                                              uint64_t arg1, uint64_t arg2,
                                              uint64_t arg3, uint64_t arg4) {
  return ATOMS_KernelService_Dispatch(service_nr, arg1, arg2, arg3, arg4);
}

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SYSCALL_GATEWAY_H
