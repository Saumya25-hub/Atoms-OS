/*
 * BOSPECTRA V3 — Leak Detector Subsystem
 * kernel/media/bospectra/resource/leak_detector.h
 *
 * Tracks live object allocations, releases, peak memory footprint, and pinpoints leaks upon session closure.
 */

#ifndef BOSPECTRA_V3_LEAK_DETECTOR_H
#define BOSPECTRA_V3_LEAK_DETECTOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t total_allocated;
    uint32_t currently_alive;
    uint32_t total_released;
    uint32_t total_destroyed;
    uint32_t peak_objects;
    uint32_t detected_leaks;
} BOSPECTRA_LeakDetectorStats;

void                        bospectra_leak_detector_init(void);
void                        bospectra_leak_detector_shutdown(void);

void                        bospectra_leak_track_alloc(uint32_t res_id);
void                        bospectra_leak_track_release(uint32_t res_id);

BOSPECTRA_LeakDetectorStats bospectra_leak_detector_get_stats(void);

#endif /* BOSPECTRA_V3_LEAK_DETECTOR_H */
