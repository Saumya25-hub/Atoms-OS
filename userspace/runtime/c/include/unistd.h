/*
 * ATOMS OS — Userspace C Runtime unistd.h
 * Adapted from musl libc (MIT License)
 */

#ifndef ATOMS_USER_UNISTD_H
#define ATOMS_USER_UNISTD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t ssize_t;
typedef int32_t pid_t;

int     open(const char *path, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     close(int fd);
int64_t lseek(int fd, int64_t offset, int whence);

pid_t   getpid(void);
int     sched_yield(void);
unsigned int sleep(unsigned int seconds);
int     usleep(uint64_t usec);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_UNISTD_H */
