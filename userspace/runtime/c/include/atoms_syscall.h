/*
 * ATOMS OS — Userspace C Runtime Syscall Interface
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 * Adapted from musl libc syscall conventions (MIT License)
 */

#ifndef ATOMS_USER_SYSCALL_H
#define ATOMS_USER_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_WRITE           0U
#define SYS_EXIT            1U
#define SYS_GETPID          2U
#define SYS_YIELD           3U
#define SYS_UPTIME          4U
#define SYS_ALLOC           5U
#define SYS_FREE            6U
#define SYS_DEBUG_PRINT     7U
#define SYS_MMAP            8U
#define SYS_MUNMAP          9U
#define SYS_MPROTECT        10U
#define SYS_FUTEX           11U
#define SYS_CLOCK_GETTIME   12U
#define SYS_NANOSLEEP       13U
#define SYS_OPEN            14U
#define SYS_READ            15U
#define SYS_GUI_CREATE_WINDOW   16U
#define SYS_GUI_DESTROY_WINDOW  17U
#define SYS_GUI_SHOW_WINDOW     18U
#define SYS_GUI_SET_BOUNDS      19U
#define SYS_GUI_MAP_SURFACE     20U
#define SYS_GUI_INVALIDATE      21U
#define SYS_GUI_POLL_EVENT      22U
#define SYS_GUI_GET_SCREEN_INFO 23U
#define SYS_CLOSE           25U
#define SYS_SEEK            26U
#define SYS_THREAD_SPAWN    27U
#define SYS_THREAD_EXIT     28U
#define SYS_WRITE_FILE      29U

#define PROT_NONE           0x0
#define PROT_READ           0x1
#define PROT_WRITE          0x2
#define PROT_EXEC           0x4

#define MAP_SHARED          0x01
#define MAP_PRIVATE         0x02
#define MAP_FIXED           0x10
#define MAP_ANONYMOUS       0x20
#define MAP_ANON            MAP_ANONYMOUS
#define MAP_FAILED          ((void*)-1)

#define FUTEX_WAIT          0
#define FUTEX_WAKE          1

static inline uint64_t __atoms_syscall0(uint64_t num) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t __atoms_syscall1(uint64_t num, uint64_t a1) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t __atoms_syscall2(uint64_t num, uint64_t a1, uint64_t a2) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t __atoms_syscall3(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(num), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t __atoms_syscall4(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    uint64_t ret;
    register uint64_t r10 __asm__("r10") = a4;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
                     : "rcx", "r11", "memory");
    return ret;
}

static inline uint64_t __atoms_syscall5(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    uint64_t ret;
    register uint64_t r10 __asm__("r10") = a4;
    register uint64_t r8  __asm__("r8")  = a5;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
                     : "rcx", "r11", "memory");
    return ret;
}


static inline uint64_t __atoms_syscall6(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6) {
    uint64_t ret;
    register uint64_t r10 __asm__("r10") = a4;
    register uint64_t r8  __asm__("r8")  = a5;
    register uint64_t r9  __asm__("r9")  = a6;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
    return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_SYSCALL_H */
