/*
 * BOSPECTRA V3 — Master Media Clock Subsystem
 * kernel/media/bospectra/scheduler/scheduler_clock.h
 *
 * Master presentation clock supporting start, pause, resume, speed scaling, seeking, and drift tracking.
 */

#ifndef BOSPECTRA_V3_SCHEDULER_CLOCK_H
#define BOSPECTRA_V3_SCHEDULER_CLOCK_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t master_clock_us;
    uint64_t start_time_us;
    uint64_t accumulated_media_time_us;
    uint32_t speed_multiplier_x100; /* 100 = 1.0x, 200 = 2.0x, 50 = 0.5x */
    int64_t  clock_drift_us;
    bool     is_running;
} BOSPECTRA_MasterClock;

void bospectra_master_clock_init(BOSPECTRA_MasterClock* clk);
void bospectra_master_clock_reset(BOSPECTRA_MasterClock* clk);

bospectra_error_t bospectra_master_clock_start(BOSPECTRA_MasterClock* clk, uint64_t start_pts_us);
bospectra_error_t bospectra_master_clock_pause(BOSPECTRA_MasterClock* clk);
bospectra_error_t bospectra_master_clock_resume(BOSPECTRA_MasterClock* clk);
bospectra_error_t bospectra_master_clock_set_speed(BOSPECTRA_MasterClock* clk, uint32_t speed_x100);
bospectra_error_t bospectra_master_clock_seek(BOSPECTRA_MasterClock* clk, uint64_t target_pts_us);
void              bospectra_master_clock_update(BOSPECTRA_MasterClock* clk, uint64_t delta_us);

uint64_t bospectra_master_clock_get_media_time(const BOSPECTRA_MasterClock* clk);

#endif /* BOSPECTRA_V3_SCHEDULER_CLOCK_H */
