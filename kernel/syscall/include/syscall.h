#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Syscall Numbers
#define SYS_YIELD  0
#define SYS_SLEEP  1
#define SYS_UPTIME 2
#define SYS_GETPID 3

// Maximum Syscall ID + 1 for validation
#define MAX_SYSCALL 4

// System Call Initialization
void syscall_init(void);

// User-facing System Call Wrappers
void sys_yield(void);
void sys_sleep(uint64_t ticks);
uint64_t sys_uptime(void);
uint64_t sys_getpid(void);

#endif // SYSCALL_H
