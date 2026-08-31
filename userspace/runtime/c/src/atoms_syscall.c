/*
 * ATOMS OS — Userspace C Runtime Syscall Wrapper Implementations
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 * Adapted from musl libc (MIT License)
 */

#include "../include/atoms_syscall.h"
#include "../include/unistd.h"
#include "../include/sys/mman.h"
#include "../include/time.h"

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset) {
    uint64_t res = __atoms_syscall6(SYS_MMAP, (uint64_t)addr, length, (uint64_t)prot, (uint64_t)flags, (uint64_t)fd, (uint64_t)offset);
    if (res == (uint64_t)-1) {
        return MAP_FAILED;
    }
    return (void *)res;
}

int munmap(void *addr, size_t length) {
    return (int)__atoms_syscall2(SYS_MUNMAP, (uint64_t)addr, length);
}

int mprotect(void *addr, size_t length, int prot) {
    return (int)__atoms_syscall3(SYS_MPROTECT, (uint64_t)addr, length, (uint64_t)prot);
}

int clock_gettime(int clock_id, struct timespec *tp) {
    return (int)__atoms_syscall2(SYS_CLOCK_GETTIME, (uint64_t)clock_id, (uint64_t)tp);
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    return (int)__atoms_syscall2(SYS_NANOSLEEP, (uint64_t)req, (uint64_t)rem);
}

int open(const char *path, int flags, ...) {
    return (int)__atoms_syscall3(SYS_OPEN, (uint64_t)path, (uint64_t)flags, 0);
}

ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t)__atoms_syscall3(SYS_READ, (uint64_t)fd, (uint64_t)buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    if (fd == 1 || fd == 2) {
        return (ssize_t)__atoms_syscall2(SYS_WRITE, (uint64_t)buf, count);
    }
    return (ssize_t)__atoms_syscall3(SYS_WRITE_FILE, (uint64_t)fd, (uint64_t)buf, count);
}

int close(int fd) {
    return (int)__atoms_syscall1(SYS_CLOSE, (uint64_t)fd);
}

int64_t lseek(int fd, int64_t offset, int whence) {
    return (int64_t)__atoms_syscall3(SYS_SEEK, (uint64_t)fd, (uint64_t)offset, (uint64_t)whence);
}

pid_t getpid(void) {
    return (pid_t)__atoms_syscall0(SYS_GETPID);
}

int sched_yield(void) {
    return (int)__atoms_syscall0(SYS_YIELD);
}

void exit(int status) {
    __atoms_syscall1(SYS_EXIT, (uint64_t)status);
    while (1) { }
}

void abort(void) {
    exit(134);
}

uint32_t sys_gui_create_window(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flags, const char* title) {
    return (uint32_t)__atoms_syscall6(SYS_GUI_CREATE_WINDOW, (uint64_t)x, (uint64_t)y, (uint64_t)w, (uint64_t)h, (uint64_t)flags, (uint64_t)title);
}

int sys_gui_destroy_window(uint32_t win_id) {
    return (int)__atoms_syscall1(SYS_GUI_DESTROY_WINDOW, (uint64_t)win_id);
}

int sys_gui_show_window(uint32_t win_id, uint32_t visible) {
    return (int)__atoms_syscall2(SYS_GUI_SHOW_WINDOW, (uint64_t)win_id, (uint64_t)visible);
}

int sys_gui_map_surface(uint32_t win_id, uint64_t* out_surface_ptr, uint32_t* out_stride_bytes) {
    return (int)__atoms_syscall3(SYS_GUI_MAP_SURFACE, (uint64_t)win_id, (uint64_t)out_surface_ptr, (uint64_t)out_stride_bytes);
}

int sys_gui_invalidate(uint32_t win_id, int32_t x, int32_t y, int32_t w, int32_t h) {
    return (int)__atoms_syscall5(SYS_GUI_INVALIDATE, (uint64_t)win_id, (uint64_t)x, (uint64_t)y, (uint64_t)w, (uint64_t)h);
}

int sys_gui_poll_event(uint32_t win_id, void* out_event, uint32_t event_size) {
    return (int)__atoms_syscall3(SYS_GUI_POLL_EVENT, (uint64_t)win_id, (uint64_t)out_event, (uint64_t)event_size);
}
