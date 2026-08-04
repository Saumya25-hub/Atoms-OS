/*
 * BOSPECTRA V3 — Certification Runner Subsystem
 * kernel/media/bospectra/certification/cert_runner.h
 */

#ifndef BOSPECTRA_V3_CERT_RUNNER_H
#define BOSPECTRA_V3_CERT_RUNNER_H

#include "cert_validator.h"

void                         bospectra_cert_runner_init(void);
void                         bospectra_cert_runner_shutdown(void);

BOSPECTRA_CertificationScore bospectra_cert_runner_execute(void);

#endif /* BOSPECTRA_V3_CERT_RUNNER_H */
