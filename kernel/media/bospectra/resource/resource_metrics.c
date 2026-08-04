/*
 * BOSPECTRA V3 — Resource Metrics Implementation
 * kernel/media/bospectra/resource/resource_metrics.c
 */

#include "resource_metrics.h"
#include "ownership_manager.h"
#include "reference_manager.h"
#include "leak_detector.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_resource_metrics_dump(void) {
    BOSPECTRA_LeakDetectorStats leak_stats = bospectra_leak_detector_get_stats();

    display_print("\n============= RESOURCE MANAGER =============\n");
    display_print("Packets / Frames Alive: ");
    bospectra_trace_u32("Alive Objects", leak_stats.currently_alive);
    display_print("Peak Objects Alive    : ");
    bospectra_trace_u32("Peak Objects", leak_stats.peak_objects);

    display_print("\nReference Count Errors: ");
    bospectra_trace_u32("Ref Errors", bospectra_ref_get_errors_count());
    display_print("Ownership Violations  : ");
    bospectra_trace_u32("Violations", bospectra_ownership_get_violations_count());
    display_print("Memory Leaks          : ");
    bospectra_trace_u32("Leaks", leak_stats.detected_leaks);
    display_print("============================================\n\n");
}
