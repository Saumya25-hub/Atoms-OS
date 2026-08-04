/*
 * BOSPECTRA V3 — Certification Engine Subsystem
 * kernel/media/bospectra/certification/cert_engine.h
 */

#ifndef BOSPECTRA_V3_CERT_ENGINE_H
#define BOSPECTRA_V3_CERT_ENGINE_H

#include "cert_validator.h"

void                         bospectra_cert_engine_init(void);
void                         bospectra_cert_engine_shutdown(void);

BOSPECTRA_CertificationScore bospectra_cert_engine_run(void);

#endif /* BOSPECTRA_V3_CERT_ENGINE_H */
