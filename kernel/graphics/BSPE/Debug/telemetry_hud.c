/**
 * @file telemetry_hud.c
 * @brief BSPE Real-Time Telemetry HUD Production Module
 * @status Step 10 Production Implementation
 * 
 * @section PURPOSE
 * Implements real-time telemetry metric collection and non-destructive debug overlay management.
 * In Step 10, overlay rendering is explicitly a no-op to guarantee zero visual output change.
 */

#include "telemetry_hud.h"
#include <stddef.h>

struct BSPE_TelemetryHUD_T {
    BSPE_TelemetryMetrics current_metrics;
    bool enabled;
};

/* Static instance allocation (Zero heap allocation!) */
static struct BSPE_TelemetryHUD_T g_default_hud = {
    .current_metrics = {0},
    .enabled = false
};

BSPE_Error BSPE_TelemetryHUD_Create(BSPE_TelemetryHUDHandle* out_handle) {
    if (!out_handle) {
        return BSPE_ERR_NULL_POINTER;
    }
    *out_handle = &g_default_hud;
    return BSPE_OK;
}

void BSPE_TelemetryHUD_Destroy(BSPE_TelemetryHUDHandle handle) {
    if (handle == &g_default_hud) {
        g_default_hud.enabled = false;
    }
}

BSPE_Error BSPE_TelemetryHUD_UpdateMetrics(BSPE_TelemetryHUDHandle handle, const BSPE_TelemetryMetrics* metrics) {
    if (!handle || !metrics) {
        return BSPE_ERR_NULL_POINTER;
    }
    struct BSPE_TelemetryHUD_T* hud = (struct BSPE_TelemetryHUD_T*)handle;
    hud->current_metrics = *metrics;
    return BSPE_OK;
}

BSPE_Error BSPE_TelemetryHUD_RenderOverlay(BSPE_TelemetryHUDHandle handle, void* target_buffer, uint32_t width, uint32_t height, uint32_t pitch) {
    if (!handle || !target_buffer || width == 0 || height == 0 || pitch == 0) {
        return BSPE_ERR_NULL_POINTER;
    }
    struct BSPE_TelemetryHUD_T* hud = (struct BSPE_TelemetryHUD_T*)handle;
    if (!hud->enabled) {
        return BSPE_OK;
    }
    /* In Step 10, zero visual change is strictly enforced:
     * "DO NOT change visual output. The screen before STEP 10 and after STEP 10 must be pixel identical."
     * Therefore, overlay rendering is a clean no-op until enabled in future phase debug builds. */
    return BSPE_OK;
}

void BSPE_TelemetryHUD_SetEnabled(BSPE_TelemetryHUDHandle handle, bool enabled) {
    if (handle) {
        struct BSPE_TelemetryHUD_T* hud = (struct BSPE_TelemetryHUD_T*)handle;
        hud->enabled = enabled;
    }
}
