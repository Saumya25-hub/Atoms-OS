/*
 * ATOMS OS — Userspace POSIX System Call Adapter
 * Connects standard POSIX system call APIs directly to ATOMS kernel syscalls.
 */

#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* Global errno storage (TLS fallback) */
static int g_errno = 0;

int *__errno_location(void) {
    return &g_errno;
}

int *___errno_location(void) {
    return &g_errno;
}

int __lockfile(void *f) {
    (void)f;
    return 1;
}

void __unlockfile(void *f) {
    (void)f;
}

void __lock(volatile int *l) {
    while (__atomic_test_and_set(l, __ATOMIC_ACQUIRE)) {}
}

void __unlock(volatile int *l) {
    __atomic_clear(l, __ATOMIC_RELEASE);
}

const char *__lctrans(const char *msg, const void *lm) {
    (void)lm;
    return msg;
}

const char *__lctrans_cur(const char *msg) {
    return msg;
}

int64_t write(int fd, const void *buf, size_t count) {
    if (!buf || count == 0) return 0;
    if (fd == 1 || fd == 2) {
        return atoms_sys_write(fd, buf, count);
    }
    return __syscall3(SYS_WRITE_FILE, (int64_t)fd, (int64_t)buf, (int64_t)count);
}

int vsnprintf(char *s, size_t n, const char *format, va_list ap);

int printf(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        write(1, buf, (size_t)n);
    }
    return n;
}

int puts(const char *s) {
    if (!s) return -1;
    size_t len = 0;
    while (s[len]) len++;
    write(1, s, len);
    write(1, "\n", 1);
    return (int)len + 1;
}

int64_t read(int fd, void *buf, size_t count) {
    if (!buf || count == 0) return 0;
    return __syscall3(SYS_READ, (int64_t)fd, (int64_t)buf, (int64_t)count);
}

int open(const char *path, int flags, ...) {
    if (!path) return -1;
    return (int)__syscall3(SYS_OPEN, (int64_t)path, (int64_t)flags, 0);
}

int close(int fd) {
    return (int)__syscall1(SYS_CLOSE, (int64_t)fd);
}

int64_t lseek(int fd, int64_t offset, int whence) {
    return __syscall3(SYS_SEEK, (int64_t)fd, offset, (int64_t)whence);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset) {
    return atoms_sys_mmap(addr, length, prot, flags, fd, offset);
}

int munmap(void *addr, size_t length) {
    return atoms_sys_munmap(addr, length);
}

int mprotect(void *addr, size_t length, int prot) {
    return atoms_sys_mprotect(addr, length, prot);
}

int futex(uint32_t *uaddr, int op, uint32_t val, const void *timeout) {
    return atoms_sys_futex(uaddr, op, val, timeout);
}

int clock_gettime(int clk_id, void *tp) {
    return (int)__syscall2(SYS_CLOCK_GETTIME, (int64_t)clk_id, (int64_t)tp);
}

int nanosleep(const void *req, void *rem) {
    return (int)__syscall2(SYS_NANOSLEEP, (int64_t)req, (int64_t)rem);
}

int sched_yield(void) {
    return (int)__syscall0(SYS_YIELD);
}

int getpid(void) {
    return (int)__syscall0(SYS_GETPID);
}

void _exit(int status) {
    atoms_sys_exit(status);
}

void exit(int status) {
    _exit(status);
}

void abort(void) {
    const char msg[] = "[ATOMS RUNTIME] Program aborted!\n";
    atoms_sys_write(2, msg, sizeof(msg) - 1);
    atoms_sys_exit(134);
}

/* Universal system call dispatcher for POSIX / Linux syscall numbers */
long syscall(long number, ...) {
    va_list ap;
    va_start(ap, number);
    long a1 = va_arg(ap, long);
    long a2 = va_arg(ap, long);
    long a3 = va_arg(ap, long);
    long a4 = va_arg(ap, long);
    long a5 = va_arg(ap, long);
    long a6 = va_arg(ap, long);
    va_end(ap);

    switch (number) {
        case 0: /* Linux read / ATOMS SYS_WRITE */
            return __syscall3(SYS_READ, a1, a2, a3);
        case 1: /* Linux write / ATOMS SYS_EXIT */
            return write((int)a1, (const void *)a2, (size_t)a3);
        case 2: /* Linux open / ATOMS SYS_GETPID */
            return open((const char *)a1, (int)a2);
        case 3: /* Linux close / ATOMS SYS_YIELD */
            return close((int)a1);
        case 9: /* Linux mmap */
            return (long)mmap((void *)a1, (size_t)a2, (int)a3, (int)a4, (int)a5, (int64_t)a6);
        case 10: /* Linux mprotect */
            return mprotect((void *)a1, (size_t)a2, (int)a3);
        case 11: /* Linux munmap */
            return munmap((void *)a1, (size_t)a2);
        case 24: /* Linux sched_yield */
            return sched_yield();
        case 35: /* Linux nanosleep */
            return nanosleep((const void *)a1, (void *)a2);
        case 39: /* Linux getpid */
        case 186: /* Linux gettid */
            return getpid();
        case 60: /* Linux exit */
        case 231: /* Linux exit_group */
            _exit((int)a1);
            return 0;
        case 202: /* Linux futex */
            return futex((uint32_t *)a1, (int)a2, (uint32_t)a3, (const void *)a4);
        case 228: /* Linux clock_gettime */
            return clock_gettime((int)a1, (void *)a2);
        default:
            return __syscall6(number, a1, a2, a3, a4, a5, a6);
    }
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
    (void)func; (void)arg; (void)dso_handle;
    return 0;
}

void __abort_message(const char *format, ...) {
    (void)format;
    const char msg[] = "[LLVM LIBCXXABI] Abort message triggered!\n";
    atoms_sys_write(2, msg, sizeof(msg) - 1);
    abort();
}
