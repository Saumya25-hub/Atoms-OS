/*
 * BOSPECTRA V3 — Sync Manager
 * kernel/media/bospectra/manager/sync_manager.h
 *
 * Master clock synchronization, PTS scheduling, and presentation timing.
 */

#ifndef BOSPECTRA_SYNC_MANAGER_H
#define BOSPECTRA_SYNC_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../sync/include/bospectra_sync.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t master_clock_us;
    uint64_t last_pts_us;
    uint32_t current_speed_x100;
    bool     is_sync_locked;
} BOSPECTRA_SyncState;

void bospectra_sync_manager_init(void);
void bospectra_sync_manager_shutdown(void);

bospectra_error_t bospectra_sync_update_clock(uint64_t pts_us);
uint64_t          bospectra_sync_get_clock(void);
bospectra_error_t bospectra_sync_set_speed(uint32_t speed_x100);
bool              bospectra_sync_should_present_frame(uint64_t frame_pts_us);

#endif /* BOSPECTRA_SYNC_MANAGER_H */
