/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Time & Clocks Implementation
 */

#include "apal_time.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"

struct apal_timespec {
    int64_t tv_sec;
    int64_t tv_nsec;
};

uint64_t apal_time_now_monotonic_ns(void) {
    struct apal_timespec ts = {0, 0};
    int64_t ret = __syscall2(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC, (int64_t)&ts);
    if (ret == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    /* Fallback to uptime in ticks */
    uint64_t uptime_ms = atoms_sys_uptime();
    return uptime_ms * 1000000ULL;
}

uint64_t apal_time_now_monotonic_us(void) {
    return apal_time_now_monotonic_ns() / 1000ULL;
}

uint64_t apal_time_now_realtime_us(void) {
    struct apal_timespec ts = {0, 0};
    int64_t ret = __syscall2(SYS_CLOCK_GETTIME, CLOCK_REALTIME, (int64_t)&ts);
    if (ret == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
    }
    return apal_time_now_monotonic_us();
}

void apal_sleep_ms(uint32_t ms) {
    struct apal_timespec req;
    req.tv_sec = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000ULL;
    __syscall2(SYS_NANOSLEEP, (int64_t)&req, 0);
}

void apal_sleep_us(uint64_t us) {
    struct apal_timespec req;
    req.tv_sec = us / 1000000ULL;
    req.tv_nsec = (us % 1000000ULL) * 1000ULL;
    __syscall2(SYS_NANOSLEEP, (int64_t)&req, 0);
}
