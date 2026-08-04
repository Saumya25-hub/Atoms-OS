/*
 * BOSPECTRA V3 — Certification Scenarios Subsystem
 * kernel/media/bospectra/certification/cert_scenarios.h
 */

#ifndef BOSPECTRA_V3_CERT_SCENARIOS_H
#define BOSPECTRA_V3_CERT_SCENARIOS_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bospectra_cert_scenarios_init(void);
void bospectra_cert_scenarios_shutdown(void);

uint32_t bospectra_cert_run_all_scenarios(void);

#endif /* BOSPECTRA_V3_CERT_SCENARIOS_H */
