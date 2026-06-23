#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Syscall Numbers
#define SYS_YIELD  0
#define SYS_WRITE  1
#define SYS_SLEEP  2
#define SYS_UPTIME 3
#define SYS_GETPID 4
#define SYS_EXIT   5
#define SYS_OPEN   6
#define SYS_READ   7
#define SYS_CLOSE  8
#define SYS_GETC   9
#define SYS_SPAWN   10
#define SYS_READDIR 11
#define SYS_HEAPINFO 12
#define SYS_PS     13
#define SYS_GET_KEY_EVENT 14

// Maximum Syscall ID + 1 for validation
#define MAX_SYSCALL 15

// System Call Initialization
extern void syscall_init_asm(void);
void syscall_init(void);

// C Handler (Called from assembly)
uint64_t syscall_handler(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

// User-facing System Call Wrappers (for Kernel Tasks)
void sys_yield(void);
void sys_sleep(uint64_t ticks);
uint64_t sys_uptime(void);
uint64_t sys_getpid(void);

#endif // SYSCALL_H
