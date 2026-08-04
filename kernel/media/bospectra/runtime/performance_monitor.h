/*
 * BOSPECTRA V3 — Performance Monitor Subsystem
 * kernel/media/bospectra/runtime/performance_monitor.h
 */

#ifndef BOSPECTRA_V3_PERFORMANCE_MONITOR_H
#define BOSPECTRA_V3_PERFORMANCE_MONITOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t current_fps;
    uint32_t average_fps;
    uint32_t target_fps;
    uint32_t frame_time_us;
    uint32_t worst_frame_time_us;
    uint32_t best_frame_time_us;
    uint32_t jitter_us;
    uint32_t pipeline_latency_us;
} BOSPECTRA_PerformanceStats;

void                       bospectra_performance_monitor_init(uint32_t target_fps);
void                       bospectra_performance_monitor_shutdown(void);

void                       bospectra_performance_record_frame(uint64_t pts_us);
BOSPECTRA_PerformanceStats bospectra_performance_get_stats(void);

#endif /* BOSPECTRA_V3_PERFORMANCE_MONITOR_H */
