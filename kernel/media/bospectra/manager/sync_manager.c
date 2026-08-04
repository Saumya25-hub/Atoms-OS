/*
 * BOSPECTRA V3 — Sync Manager Implementation
 * kernel/media/bospectra/manager/sync_manager.c
 */

#include "sync_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_SyncState g_sync_state;
static bool g_sync_manager_initialized = false;

void bospectra_sync_manager_init(void) {
    memset(&g_sync_state, 0, sizeof(BOSPECTRA_SyncState));
    g_sync_state.current_speed_x100 = 100;
    g_sync_state.is_sync_locked = true;
    g_sync_manager_initialized = true;
    BOSPECTRA_Sync_Init();
    bospectra_log("SYNC_MANAGER", "BOSPECTRA V3 Sync Manager initialized.");
}

void bospectra_sync_manager_shutdown(void) {
    BOSPECTRA_Sync_Shutdown();
    memset(&g_sync_state, 0, sizeof(BOSPECTRA_SyncState));
    g_sync_manager_initialized = false;
}

bospectra_error_t bospectra_sync_update_clock(uint64_t pts_us) {
    if (!g_sync_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    g_sync_state.last_pts_us = pts_us;
    g_sync_state.master_clock_us = pts_us;
    BOSPECTRA_Sync_UpdateAudioClock(pts_us);
    return BOSPECTRA_SUCCESS;
}

uint64_t bospectra_sync_get_clock(void) {
    if (!g_sync_manager_initialized) return 0;
    return g_sync_state.master_clock_us;
}

bospectra_error_t bospectra_sync_set_speed(uint32_t speed_x100) {
    if (!g_sync_manager_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    g_sync_state.current_speed_x100 = speed_x100 ? speed_x100 : 100;
    BOSPECTRA_Sync_SetPlaybackSpeed(g_sync_state.current_speed_x100);
    return BOSPECTRA_SUCCESS;
}

bool bospectra_sync_should_present_frame(uint64_t frame_pts_us) {
    (void)frame_pts_us;
    if (!g_sync_manager_initialized) return true;
    return true; /* Normal pacing */
}
