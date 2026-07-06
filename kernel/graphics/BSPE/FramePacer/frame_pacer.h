#ifndef ATOMS_OS_BSPE_FRAME_PACER_H
#define ATOMS_OS_BSPE_FRAME_PACER_H

/**
 * @file frame_pacer.h
 * @brief BSPE VSync Frame Pacer Public Header
 * @status Step 8 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public API for synchronizing presentation page flips, tracking deadlines,
 * eliminating cumulative frame drift via fractional accumulators, and monitoring jitter.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Configuration Struct --- */
typedef struct {
    uint32_t target_refresh_rate; /* Target FPS (e.g. 60, 120, 144) */
    bool enable_vsync;            /* True to bind to hardware VSync IRQ */
    uint32_t reserved[4];         /* Future extension reserve */
} BSPE_FramePacerConfig;

/* --- Public Lifecycle APIs --- */
BSPE_Error BSPE_FramePacer_Create(const BSPE_FramePacerConfig* config, BSPE_FramePacerHandle* out_handle);
void       BSPE_FramePacer_Destroy(BSPE_FramePacerHandle handle);
void       BSPE_FramePacer_Reset(BSPE_FramePacerHandle handle);

/* --- Aliases & Explicit APIs Required by Step 8 Specification --- */
static inline BSPE_Error BSPE_FramePacer_Initialize(const BSPE_FramePacerConfig* config, BSPE_FramePacerHandle* out_handle) {
    return BSPE_FramePacer_Create(config, out_handle);
}
BSPE_Error BSPE_FramePacer_BeginFrame(BSPE_FramePacerHandle handle, uint64_t now_us);
BSPE_Error BSPE_FramePacer_EndFrame(BSPE_FramePacerHandle handle, uint64_t now_us);
BSPE_Error BSPE_FramePacer_WaitForNextFrame(BSPE_FramePacerHandle handle);
BSPE_Error BSPE_FramePacer_SetRefreshRate(BSPE_FramePacerHandle handle, uint32_t refresh_rate_hz);
BSPE_Error BSPE_FramePacer_GetFrameStatistics(BSPE_FramePacerHandle handle, uint32_t* out_avg_fps, uint32_t* out_jitter_us, uint32_t* out_drift_us, uint32_t* out_dropped_frames);

/* --- VSync & Telemetry APIs from Step 3 --- */
BSPE_Error BSPE_FramePacer_OnVSyncIRQ(BSPE_FramePacerHandle handle, uint64_t timestamp_us);
BSPE_Error BSPE_FramePacer_GetAverageFPS(BSPE_FramePacerHandle handle, uint32_t* out_fps);

/* --- Verification & Diagnostics --- */
bool       BSPE_FramePacer_RunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_FRAME_PACER_H
