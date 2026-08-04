/*
 * BOSPECTRA V3 — Error Manager Implementation
 * kernel/media/bospectra/runtime/error_manager.c
 */

#include "error_manager.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static bool g_error_mgr_initialized = false;

void bospectra_error_manager_init(void) {
    g_error_mgr_initialized = true;
    bospectra_log("ERROR_MANAGER", "BOSPECTRA V3 Error Manager Initialized.");
}

void bospectra_error_manager_shutdown(void) {
    g_error_mgr_initialized = false;
}

BOSPECTRA_Error bospectra_error_create(
    bospectra_error_t code,
    BOSPECTRA_ErrorSeverity severity,
    BOSPECTRA_ErrorCategory category,
    const char* subsystem,
    const char* message,
    const char* source_file,
    const char* function,
    uint32_t line,
    uint32_t session_id,
    bool recoverable,
    const char* suggested_action
) {
    BOSPECTRA_Error err;
    memset(&err, 0, sizeof(err));
    err.error_code = code;
    err.severity = severity;
    err.category = category;
    if (subsystem) strncpy(err.subsystem, subsystem, sizeof(err.subsystem) - 1);
    if (message) strncpy(err.message, message, sizeof(err.message) - 1);
    if (source_file) strncpy(err.source_file, source_file, sizeof(err.source_file) - 1);
    if (function) strncpy(err.function, function, sizeof(err.function) - 1);
    err.line = line;
    err.session_id = session_id;
    err.recoverable = recoverable;
    if (suggested_action) strncpy(err.suggested_action, suggested_action, sizeof(err.suggested_action) - 1);

    return err;
}
