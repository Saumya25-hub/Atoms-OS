/*
 * BOSPECTRA V3 — Diagnostic Console Implementation
 * kernel/media/bospectra/diagnostics/diagnostic_console.c
 */

#include "diagnostic_console.h"
#include "media_debugger.h"
#include "pipeline_validator.h"
#include "memory_validator.h"
#include "ownership_dump.h"
#include "resource_dump.h"
#include "trace_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_diag_console_initialized = false;

void bospectra_diagnostic_console_init(void) {
    g_diag_console_initialized = true;
    bospectra_log("DIAG_CONSOLE", "BOSPECTRA V3 Diagnostic Console Initialized.");
}

void bospectra_diagnostic_console_shutdown(void) {
    g_diag_console_initialized = false;
}

bospectra_error_t bospectra_diag_dispatch_command(const char* cmd) {
    if (!cmd) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (strcmp(cmd, "bospectra diag") == 0 || strcmp(cmd, "diag") == 0) {
        bospectra_media_debugger_inspect_all();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra validate") == 0 || strcmp(cmd, "validate") == 0) {
        (void)bospectra_pipeline_validate();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra leaks") == 0 || strcmp(cmd, "leaks") == 0) {
        bospectra_memory_validator_dump_leaks();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra ownership") == 0 || strcmp(cmd, "ownership") == 0) {
        bospectra_dump_ownership_tree();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra resources") == 0 || strcmp(cmd, "resources") == 0) {
        bospectra_dump_resources();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra trace") == 0 || strcmp(cmd, "trace") == 0) {
        bospectra_trace_dump();
        return BOSPECTRA_SUCCESS;
    }
    if (strcmp(cmd, "bospectra health") == 0 || strcmp(cmd, "health") == 0) {
        (void)bospectra_pipeline_validate();
        return BOSPECTRA_SUCCESS;
    }

    display_print("\nUnknown Diagnostic Command: ");
    display_print(cmd);
    display_print("\nValid commands: diag, validate, leaks, ownership, resources, trace, health\n");
    return BOSPECTRA_ERR_INVALID_ARGUMENT;
}
