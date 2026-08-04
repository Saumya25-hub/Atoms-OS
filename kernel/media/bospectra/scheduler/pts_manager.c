/*
 * BOSPECTRA V3 — PTS Manager Implementation
 * kernel/media/bospectra/scheduler/pts_manager.c
 */

#include "pts_manager.h"
#include "kernel/core/lib/include/string.h"

void bospectra_pts_manager_init(BOSPECTRA_PTSManager* pts_mgr) {
    if (!pts_mgr) return;
    memset(pts_mgr, 0, sizeof(BOSPECTRA_PTSManager));
}

void bospectra_pts_manager_reset(BOSPECTRA_PTSManager* pts_mgr) {
    if (!pts_mgr) return;
    memset(pts_mgr, 0, sizeof(BOSPECTRA_PTSManager));
}

bospectra_error_t bospectra_pts_validate(BOSPECTRA_PTSManager* pts_mgr, uint64_t frame_pts_us) {
    if (!pts_mgr) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    pts_mgr->total_pts_evaluated++;

    if (pts_mgr->total_pts_evaluated > 1) {
        if (frame_pts_us == pts_mgr->last_pts_us) {
            pts_mgr->duplicate_pts_count++;
        } else if (frame_pts_us < pts_mgr->last_pts_us) {
            pts_mgr->monotonic_violations++;
        }
    }

    pts_mgr->last_pts_us = frame_pts_us;
    return BOSPECTRA_SUCCESS;
}
