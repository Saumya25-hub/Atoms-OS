/*
 * BOSPECTRA V3 — Certification Runner Implementation
 * kernel/media/bospectra/certification/cert_runner.c
 */

#include "cert_runner.h"
#include "cert_scenarios.h"
#include "../debug/bospectra_debug.h"

static bool g_cert_runner_initialized = false;

void bospectra_cert_runner_init(void) {
    bospectra_cert_scenarios_init();
    g_cert_runner_initialized = true;
    bospectra_log("CERT_RUNNER", "BOSPECTRA V3 Certification Runner Initialized.");
}

void bospectra_cert_runner_shutdown(void) {
    g_cert_runner_initialized = false;
}

BOSPECTRA_CertificationScore bospectra_cert_runner_execute(void) {
    if (!g_cert_runner_initialized) return bospectra_cert_evaluate_score(0, 14);
    uint32_t passed = bospectra_cert_run_all_scenarios();
    return bospectra_cert_evaluate_score(passed, 14);
}
