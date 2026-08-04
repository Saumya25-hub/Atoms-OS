/*
 * BOSPECTRA V3 — Error Dispatcher Implementation
 * kernel/media/bospectra/runtime/error_dispatcher.c
 */

#include "error_dispatcher.h"
#include "error_reporter.h"
#include "../diagnostics/trace_engine.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static uint32_t g_total_errors_dispatched = 0;
static bool     g_error_dispatcher_initialized = false;

void bospectra_error_dispatcher_init(void) {
    g_total_errors_dispatched = 0;
    g_error_dispatcher_initialized = true;
    bospectra_log("ERROR_DISPATCHER", "BOSPECTRA V3 Error Dispatcher Initialized.");
}

void bospectra_error_dispatcher_shutdown(void) {
    g_error_dispatcher_initialized = false;
}

bospectra_error_t bospectra_error_dispatch(const BOSPECTRA_Error* err) {
    if (!g_error_dispatcher_initialized || !err) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    g_total_errors_dispatched++;
    bospectra_trace_record(err->session_id, err->subsystem, err->message, (uint32_t)err->error_code);

    if (err->severity >= ERR_SEVERITY_ERROR) {
        bospectra_error_report_print(err);
    }
    return BOSPECTRA_SUCCESS;
}

uint32_t bospectra_error_get_total_count(void) {
    return g_total_errors_dispatched;
}
