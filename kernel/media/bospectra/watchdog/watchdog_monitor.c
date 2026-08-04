/*
 * BOSPECTRA V3 — Watchdog Pipeline Monitor Implementation
 * kernel/media/bospectra/watchdog/watchdog_monitor.c
 */

#include "watchdog_monitor.h"
#include "../debug/bospectra_debug.h"

static bool g_monitor_initialized = false;

void bospectra_watchdog_monitor_init(void) {
    g_monitor_initialized = true;
    bospectra_log("WATCHDOG_MONITOR", "BOSPECTRA V3 Pipeline Stage Monitor Initialized.");
}

void bospectra_watchdog_monitor_shutdown(void) {
    g_monitor_initialized = false;
}

BOSPECTRA_PipelineStage bospectra_watchdog_get_current_stage(void) {
    return STAGE_DISPLAY;
}

const char* bospectra_stage_to_string(BOSPECTRA_PipelineStage stage) {
    switch (stage) {
        case STAGE_DISK:    return "Disk (VFS)";
        case STAGE_PACKET:  return "Packet Queue";
        case STAGE_DECODE:  return "Decoder";
        case STAGE_FRAME:   return "Frame Queue";
        case STAGE_COLOR:   return "Color Engine";
        case STAGE_TEXTURE: return "Texture Pool";
        case STAGE_SURFACE: return "Surface Pool";
        case STAGE_BWE:     return "BWE Connection";
        case STAGE_DISPLAY: return "Display Scheduler";
        default:            return "Unknown Stage";
    }
}
