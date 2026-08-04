/*
 * BOSPECTRA V3 — Error Reporter Implementation
 * kernel/media/bospectra/runtime/error_reporter.c
 */

#include "error_reporter.h"
#include "../debug/bospectra_debug.h"

extern void display_print(const char* str);

void bospectra_error_report_print(const BOSPECTRA_Error* err) {
    if (!err) return;

    display_print("\n============= BOSPECTRA ERROR REPORT =============\n");
    display_print("Subsystem        : ");
    display_print(err->subsystem);
    display_print("\nSeverity         : ");
    const char* severities[] = { "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL" };
    display_print(severities[err->severity % 5]);
    display_print("\nMessage          : ");
    display_print(err->message);
    display_print("\nSession ID       : ");
    bospectra_trace_u32("ID", err->session_id);
    display_print("Recoverable      : ");
    display_print(err->recoverable ? "YES\n" : "NO\n");
    display_print("Suggested Action : ");
    display_print(err->suggested_action);
    display_print("\n==================================================\n\n");
}
