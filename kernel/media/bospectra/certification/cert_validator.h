/*
 * BOSPECTRA V3 — Certification Validator Subsystem
 * kernel/media/bospectra/certification/cert_validator.h
 */

#ifndef BOSPECTRA_V3_CERT_VALIDATOR_H
#define BOSPECTRA_V3_CERT_VALIDATOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t score_percentage;
    bool     production_ready;
} BOSPECTRA_CertificationScore;

void                          bospectra_cert_validator_init(void);
void                          bospectra_cert_validator_shutdown(void);

BOSPECTRA_CertificationScore bospectra_cert_evaluate_score(uint32_t passed, uint32_t total);

#endif /* BOSPECTRA_V3_CERT_VALIDATOR_H */
