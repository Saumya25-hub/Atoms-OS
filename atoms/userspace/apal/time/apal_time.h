/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Time & Clocks Adapter (Chromium base::Time / base::TimeTicks)
 */

#ifndef ATOMS_APAL_TIME_H
#define ATOMS_APAL_TIME_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Monotonic time in microseconds since system boot */
uint64_t apal_time_now_monotonic_us(void);

/* Monotonic time in nanoseconds since system boot */
uint64_t apal_time_now_monotonic_ns(void);

/* Wall-clock time in microseconds since UNIX epoch */
uint64_t apal_time_now_realtime_us(void);

/* Sleep current thread for specified milliseconds */
void apal_sleep_ms(uint32_t ms);

/* Sleep current thread for specified microseconds */
void apal_sleep_us(uint64_t us);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_TIME_H */
