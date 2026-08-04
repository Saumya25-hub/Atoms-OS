/*
 * BOSPECTRA V3 — PTS Manager Subsystem
 * kernel/media/bospectra/scheduler/pts_manager.h
 *
 * Validates monotonic timestamps, missing timestamps, duplicate timestamps, and invalid timestamps.
 */

#ifndef BOSPECTRA_V3_PTS_MANAGER_H
#define BOSPECTRA_V3_PTS_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t last_pts_us;
    uint32_t monotonic_violations;
    uint32_t missing_pts_count;
    uint32_t duplicate_pts_count;
    uint32_t total_pts_evaluated;
} BOSPECTRA_PTSManager;

void bospectra_pts_manager_init(BOSPECTRA_PTSManager* pts_mgr);
void bospectra_pts_manager_reset(BOSPECTRA_PTSManager* pts_mgr);

bospectra_error_t bospectra_pts_validate(BOSPECTRA_PTSManager* pts_mgr, uint64_t frame_pts_us);

#endif /* BOSPECTRA_V3_PTS_MANAGER_H */
