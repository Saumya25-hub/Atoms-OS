/*
 * BOSPECTRA V3 — Master Media Clock Implementation
 * kernel/media/bospectra/scheduler/scheduler_clock.c
 */

#include "scheduler_clock.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

void bospectra_master_clock_init(BOSPECTRA_MasterClock* clk) {
    if (!clk) return;
    memset(clk, 0, sizeof(BOSPECTRA_MasterClock));
    clk->speed_multiplier_x100 = 100;
}

void bospectra_master_clock_reset(BOSPECTRA_MasterClock* clk) {
    if (!clk) return;
    memset(clk, 0, sizeof(BOSPECTRA_MasterClock));
    clk->speed_multiplier_x100 = 100;
}

bospectra_error_t bospectra_master_clock_start(BOSPECTRA_MasterClock* clk, uint64_t start_pts_us) {
    if (!clk) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    clk->master_clock_us = start_pts_us;
    clk->accumulated_media_time_us = start_pts_us;
    clk->is_running = true;
    bospectra_log("MASTER_CLOCK", "Master Clock Started.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_master_clock_pause(BOSPECTRA_MasterClock* clk) {
    if (!clk) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    clk->is_running = false;
    bospectra_log("MASTER_CLOCK", "Master Clock Paused.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_master_clock_resume(BOSPECTRA_MasterClock* clk) {
    if (!clk) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    clk->is_running = true;
    bospectra_log("MASTER_CLOCK", "Master Clock Resumed.");
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_master_clock_set_speed(BOSPECTRA_MasterClock* clk, uint32_t speed_x100) {
    if (!clk || speed_x100 == 0) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    clk->speed_multiplier_x100 = speed_x100;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t bospectra_master_clock_seek(BOSPECTRA_MasterClock* clk, uint64_t target_pts_us) {
    if (!clk) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    clk->master_clock_us = target_pts_us;
    clk->accumulated_media_time_us = target_pts_us;
    return BOSPECTRA_SUCCESS;
}

void bospectra_master_clock_update(BOSPECTRA_MasterClock* clk, uint64_t delta_us) {
    if (!clk || !clk->is_running) return;
    uint64_t scaled_delta = (delta_us * clk->speed_multiplier_x100) / 100;
    clk->master_clock_us += scaled_delta;
    clk->accumulated_media_time_us += scaled_delta;
}

uint64_t bospectra_master_clock_get_media_time(const BOSPECTRA_MasterClock* clk) {
    return clk ? clk->master_clock_us : 0;
}
