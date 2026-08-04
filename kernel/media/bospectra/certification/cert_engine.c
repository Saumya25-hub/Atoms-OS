/*
 * BOSPECTRA V3 — Certification Engine Implementation
 * kernel/media/bospectra/certification/cert_engine.c
 */

#include "cert_engine.h"
#include "cert_runner.h"
#include "../debug/bospectra_debug.h"

static bool g_cert_engine_initialized = false;

void bospectra_cert_engine_init(void) {
    bospectra_cert_validator_init();
    bospectra_cert_runner_init();
    g_cert_engine_initialized = true;
    bospectra_log("CERT_ENGINE", "BOSPECTRA V3 Multimedia Certification Laboratory Initialized.");
}

void bospectra_cert_engine_shutdown(void) {
    g_cert_engine_initialized = false;
}

BOSPECTRA_CertificationScore bospectra_cert_engine_run(void) {
    if (!g_cert_engine_initialized) return bospectra_cert_evaluate_score(0, 14);
    return bospectra_cert_runner_execute();
}
