#ifndef BOSPECTRA_SYNC_H
#define BOSPECTRA_SYNC_H

#include "../../include/bospectra_types.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "bospectra_sync_types.h"

typedef struct {
    uint64_t                 master_clock_us;
    uint64_t                 current_audio_pts;
    uint64_t                 current_video_pts;
    int64_t                  current_drift_us;
    int64_t                  avg_drift_us;
    int64_t                  max_drift_us;
    uint64_t                 frames_presented;
    uint64_t                 frames_dropped;
    uint64_t                 frames_delayed;
    uint32_t                 playback_speed_x100; // 100 = 1.0x
    bospectra_clock_source_t active_clock_source;
} BOSPECTRA_SyncStats;

// Master Synchronization Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Sync_Init(void);
bospectra_error_t BOSPECTRA_Sync_Shutdown(void);

// Clock & Timing Controls
bospectra_error_t BOSPECTRA_Sync_SetClockSource(bospectra_clock_source_t source);
bospectra_error_t BOSPECTRA_Sync_UpdateAudioClock(uint64_t audio_pts_us);
uint64_t          BOSPECTRA_Sync_GetMasterClock(void);
bospectra_error_t BOSPECTRA_Sync_SetPlaybackSpeed(uint32_t speed_x100);

// Frame Presentation Scheduler Interface
bospectra_sync_decision_t BOSPECTRA_Sync_EvaluateFrame(uint64_t frame_pts_us);

// Telemetry & Diagnostics Query
void BOSPECTRA_Sync_GetStats(BOSPECTRA_SyncStats* out_stats);
void BOSPECTRA_Sync_DumpDiagnostics(void);

#endif // BOSPECTRA_SYNC_H
