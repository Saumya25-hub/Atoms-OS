/*
 * ATOMS OS — Userspace C Runtime sys/mman.h
 * Adapted from musl libc (MIT License)
 */

#ifndef ATOMS_USER_SYS_MMAN_H
#define ATOMS_USER_SYS_MMAN_H

#include <stddef.h>
#include <stdint.h>
#include "../atoms_syscall.h"

#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t length, int prot, int flags, int fd, int64_t offset);
int   munmap(void *addr, size_t length);
int   mprotect(void *addr, size_t length, int prot);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_SYS_MMAN_H */
