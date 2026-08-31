/*
 * ATOMS OS — Userspace C Runtime time.h
 * Adapted from musl libc (MIT License)
 */

#ifndef ATOMS_USER_TIME_H
#define ATOMS_USER_TIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t time_t;

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3

int clock_gettime(int clock_id, struct timespec *tp);
int nanosleep(const struct timespec *req, struct timespec *rem);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_TIME_H */
