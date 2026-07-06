#ifndef ATOMS_OS_BSPE_TELEMETRY_HUD_H
#define ATOMS_OS_BSPE_TELEMETRY_HUD_H

/**
 * @file telemetry_hud.h
 * @brief BSPE Real-Time Telemetry HUD Public Header
 * @status Step 3 API Freeze (Zero Implementation Logic)
 * 
 * @section PURPOSE
 * Defines the public API for collecting real-time presentation performance metrics
 * and rendering non-destructive graphical diagnostic overlays onto staging buffers.
 * 
 * @section DEPENDENCY_LIST
 * - "../include/bspe.h" (for BSPE_Error, BSPE_TelemetryHUDHandle)
 * - Zero circular includes.
 * 
 * @section MEMORY_OWNERSHIP
 * - BSPE_TelemetryMetrics struct pointers passed to UpdateMetrics are caller-owned read-only buffers.
 * - Telemetry handle is owned by caller until BSPE_TelemetryHUD_Destroy.
 * 
 * @section THREAD_OWNERSHIP
 * - UpdateMetrics is thread-safe and can be called from any BSPE subsystem thread.
 * - RenderOverlay is called exclusively by the BSPE Presentation Thread prior to VRAM copying.
 * 
 * @section LIFETIME_RULES
 * - Metrics history is maintained across frames until reset or destroyed.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Telemetry Metrics Struct --- */
typedef struct {
    uint32_t fps;
    uint32_t frame_time_us;
    uint32_t vram_bytes_copied;
    uint32_t dirty_rect_count;
    uint32_t cursor_latency_us;
    uint32_t present_queue_depth;
    uint32_t reserved[4]; /* Future extension reserve */
} BSPE_TelemetryMetrics;

/* --- Public Telemetry HUD APIs --- */
BSPE_Error BSPE_TelemetryHUD_Create(BSPE_TelemetryHUDHandle* out_handle);
void BSPE_TelemetryHUD_Destroy(BSPE_TelemetryHUDHandle handle);
BSPE_Error BSPE_TelemetryHUD_UpdateMetrics(BSPE_TelemetryHUDHandle handle, const BSPE_TelemetryMetrics* metrics);
BSPE_Error BSPE_TelemetryHUD_RenderOverlay(BSPE_TelemetryHUDHandle handle, void* target_buffer, uint32_t width, uint32_t height, uint32_t pitch);
void BSPE_TelemetryHUD_SetEnabled(BSPE_TelemetryHUDHandle handle, bool enabled);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_TELEMETRY_HUD_H
