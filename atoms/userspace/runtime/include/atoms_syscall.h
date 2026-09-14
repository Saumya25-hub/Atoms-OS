#ifndef ATOMS_USERSPACE_SYSCALL_H
#define ATOMS_USERSPACE_SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ATOMS OS Syscall Numbers (Matching kernel/core/syscall/include/syscall.h) */
#define SYS_WRITE           0ULL
#define SYS_EXIT            1ULL
#define SYS_GETPID          2ULL
#define SYS_YIELD           3ULL
#define SYS_UPTIME          4ULL
#define SYS_ALLOC           5ULL
#define SYS_FREE            6ULL
#define SYS_DEBUG_PRINT     7ULL
#define SYS_MMAP            8ULL
#define SYS_MUNMAP          9ULL
#define SYS_MPROTECT        10ULL
#define SYS_FUTEX           11ULL
#define SYS_CLOCK_GETTIME   12ULL
#define SYS_NANOSLEEP       13ULL
#define SYS_OPEN            14ULL
#define SYS_READ            15ULL
#define SYS_CLOSE           25ULL
#define SYS_SEEK            26ULL
#define SYS_THREAD_SPAWN    27ULL
#define SYS_THREAD_EXIT     28ULL
#define SYS_WRITE_FILE      29ULL
#define SYS_CREATE          30ULL
#define SYS_MKDIR           31ULL
#define SYS_READDIR         32ULL
#define SYS_UNLINK          33ULL
#define SYS_RENAME          34ULL
#define SYS_RMDIR           35ULL
#define SYS_STAT            36ULL
#define SYS_EXEC            37ULL
#define SYS_WAITPID         38ULL
#define SYS_IPC_CALL        39ULL
#define SYS_SHM_CALL        40ULL
#define SYS_KILL            41ULL
#define SYS_PROCESS_STATUS  42ULL

/* Syscall IPC Sub-operations */
#define ATOMS_IPC_OP_CREATE   1U
#define ATOMS_IPC_OP_CONNECT  2U
#define ATOMS_IPC_OP_SEND     3U
#define ATOMS_IPC_OP_RECV     4U
#define ATOMS_IPC_OP_CLOSE    5U

/* Syscall SHM Sub-operations */
#define ATOMS_SHM_OP_CREATE   1U
#define ATOMS_SHM_OP_OPEN     2U
#define ATOMS_SHM_OP_MAP      3U
#define ATOMS_SHM_OP_UNMAP    4U
#define ATOMS_SHM_OP_DESTROY  5U

typedef struct {
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t state;
    int32_t  exit_code;
    uint32_t thread_count;
    uint64_t cpu_time_ms;
} atoms_process_status_t;

/* Phase 9 ABI Structures */
typedef struct {
    uint64_t st_ino;        /* Inode number */
    uint32_t st_mode;       /* File mode (type + permissions) */
    uint32_t st_nlink;      /* Number of hard links (1 for BOFS) */
    uint32_t st_uid;        /* User ID of owner */
    uint32_t st_gid;        /* Group ID of owner */
    uint64_t st_size;       /* Total size in bytes (64-bit safe) */
    uint64_t st_blocks;     /* Number of 512B blocks allocated */
    uint64_t st_atime;      /* Time of last access */
    uint64_t st_mtime;      /* Time of last modification */
    uint64_t st_ctime;      /* Time of last status change */
} atoms_stat_t;

typedef struct {
    uint64_t d_ino;         /* Inode number */
    uint32_t d_type;        /* File type: 1=REG, 2=DIR */
    uint32_t d_namlen;      /* Length of name in bytes */
    char     d_name[256];   /* Null-terminated name */
} atoms_dirent_t;

/* Memory Protection Flags */
#define PROT_NONE           0x0
#define PROT_READ           0x1
#define PROT_WRITE          0x2
#define PROT_EXEC           0x4

/* Map Flags */
#define MAP_SHARED          0x01
#define MAP_PRIVATE         0x02
#define MAP_FIXED           0x10
#define MAP_ANONYMOUS       0x20
#define MAP_ANON            MAP_ANONYMOUS

/* Futex Operations */
#define FUTEX_WAIT          0
#define FUTEX_WAKE          1

/* Clock Identifiers */
#define CLOCK_REALTIME      0
#define CLOCK_MONOTONIC     1

/* Raw Inline Assembly Syscall Primitives for x86_64 Long Mode */
static inline int64_t __syscall0(int64_t n) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall1(int64_t n, int64_t a1) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall2(int64_t n, int64_t a1, int64_t a2) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall3(int64_t n, int64_t a1, int64_t a2, int64_t a3) {
    int64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall4(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4) {
    int64_t ret;
    register int64_t r10 __asm__("r10") = a4;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall5(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5) {
    int64_t ret;
    register int64_t r10 __asm__("r10") = a4;
    register int64_t r8  __asm__("r8")  = a5;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline int64_t __syscall6(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4, int64_t a5, int64_t a6) {
    int64_t ret;
    register int64_t r10 __asm__("r10") = a4;
    register int64_t r8  __asm__("r8")  = a5;
    register int64_t r9  __asm__("r9")  = a6;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

/* User-Friendly C Bindings */
static inline int64_t atoms_sys_write(int fd, const void *buf, size_t count) {
    (void)fd;
    return __syscall2(SYS_WRITE, (int64_t)buf, (int64_t)count);
}

static inline void atoms_sys_exit(int status) {
    __syscall1(SYS_EXIT, (int64_t)status);
    while (1) { __asm__ volatile("hlt"); }
}

static inline void *atoms_sys_mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset) {
    int64_t res = __syscall6(SYS_MMAP, (int64_t)addr, (int64_t)length, (int64_t)prot, (int64_t)flags, (int64_t)fd, offset);
    if (res < 0) return (void *)-1;
    return (void *)res;
}

static inline int atoms_sys_munmap(void *addr, size_t length) {
    return (int)__syscall2(SYS_MUNMAP, (int64_t)addr, (int64_t)length);
}

static inline int atoms_sys_mprotect(void *addr, size_t length, int prot) {
    return (int)__syscall3(SYS_MPROTECT, (int64_t)addr, (int64_t)length, (int64_t)prot);
}

static inline int atoms_sys_futex(uint32_t *uaddr, int futex_op, uint32_t val, const void *timeout) {
    return (int)__syscall4(SYS_FUTEX, (int64_t)uaddr, (int64_t)futex_op, (int64_t)val, (int64_t)timeout);
}

static inline int atoms_sys_thread_spawn(void (*entry)(void *), void *stack_top, void *arg) {
    return (int)__syscall3(SYS_THREAD_SPAWN, (int64_t)entry, (int64_t)stack_top, (int64_t)arg);
}

static inline void atoms_sys_thread_exit(void) {
    __syscall0(SYS_THREAD_EXIT);
    while (1) { __asm__ volatile("hlt"); }
}

static inline void atoms_sys_yield(void) {
    __syscall0(SYS_YIELD);
}

static inline int64_t atoms_sys_getpid(void) {
    return __syscall0(SYS_GETPID);
}

static inline uint64_t atoms_sys_uptime(void) {
    return (uint64_t)__syscall0(SYS_UPTIME);
}

static inline int64_t atoms_sys_waitpid(uint32_t pid, int32_t *out_status, uint32_t options) {
    return __syscall3(SYS_WAITPID, (int64_t)pid, (int64_t)out_status, (int64_t)options);
}

static inline int64_t atoms_sys_kill(uint32_t pid, int32_t signal) {
    return __syscall2(SYS_KILL, (int64_t)pid, (int64_t)signal);
}

static inline int64_t atoms_sys_process_status(uint32_t pid, atoms_process_status_t *out_status) {
    return __syscall2(SYS_PROCESS_STATUS, (int64_t)pid, (int64_t)out_status);
}

static inline int64_t atoms_sys_ipc_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3) {
    return __syscall4(SYS_IPC_CALL, (int64_t)op, (int64_t)a1, (int64_t)a2, (int64_t)a3);
}

static inline int64_t atoms_sys_shm_call(uint32_t op, uint64_t a1, uint64_t a2, uint64_t a3) {
    return __syscall4(SYS_SHM_CALL, (int64_t)op, (int64_t)a1, (int64_t)a2, (int64_t)a3);
}

#endif /* ATOMS_USERSPACE_SYSCALL_H */
