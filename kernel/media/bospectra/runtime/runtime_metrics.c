/*
 * BOSPECTRA V3 — Runtime Metrics Implementation
 * kernel/media/bospectra/runtime/runtime_metrics.c
 */

#include "runtime_metrics.h"
#include "telemetry_engine.h"
#include "performance_monitor.h"
#include "runtime_health.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_runtime_metrics_dump(void) {
    BOSPECTRA_TelemetryData telemetry = bospectra_telemetry_get_data();
    BOSPECTRA_PerformanceStats perf = bospectra_performance_get_stats();
    BOSPECTRA_HealthState health = bospectra_runtime_health_get_state();

    display_print("\n================ BOSPECTRA TELEMETRY ================\n");
    display_print("Playback FPS     : ");
    bospectra_trace_u32("FPS", perf.current_fps);
    display_print("Target FPS       : ");
    bospectra_trace_u32("Target FPS", perf.target_fps);

    display_print("\nFrames Decoded   : ");
    bospectra_trace_u32("Decoded", telemetry.frames_decoded);
    display_print("Frames Presented : ");
    bospectra_trace_u32("Presented", telemetry.frames_presented);
    display_print("Frames Dropped   : ");
    bospectra_trace_u32("Dropped", telemetry.frames_dropped);

    display_print("\nPipeline Health  : ");
    display_print(bospectra_health_to_string(health));
    display_print("\n=====================================================\n\n");
}
