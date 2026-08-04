/*
 * BOSPECTRA V3 — Certification Console Implementation
 * kernel/media/bospectra/certification/cert_console.c
 */

#include "cert_console.h"
#include "cert_engine.h"
#include "cert_report.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_cert_console_initialized = false;

void bospectra_cert_console_init(void) {
    g_cert_console_initialized = true;
    bospectra_log("CERT_CONSOLE", "BOSPECTRA V3 Certification Console Initialized.");
}

void bospectra_cert_console_shutdown(void) {
    g_cert_console_initialized = false;
}

bospectra_error_t bospectra_cert_console_dispatch(const char* cmd) {
    if (!cmd) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    if (strcmp(cmd, "cert") == 0 || strcmp(cmd, "cert run") == 0 || strcmp(cmd, "run") == 0) {
        BOSPECTRA_CertificationScore score = bospectra_cert_engine_run();
        bospectra_cert_report_print(&score);
        return BOSPECTRA_SUCCESS;
    }

    display_print("\nUnknown Certification Command: ");
    display_print(cmd);
    display_print("\nValid commands: cert, run, report, stress, codecs, containers\n");
    return BOSPECTRA_ERR_INVALID_ARGUMENT;
}
