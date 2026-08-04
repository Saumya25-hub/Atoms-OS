/*
 * BOSPECTRA V3 — Certification Console Subsystem
 * kernel/media/bospectra/certification/cert_console.h
 */

#ifndef BOSPECTRA_V3_CERT_CONSOLE_H
#define BOSPECTRA_V3_CERT_CONSOLE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"

void              bospectra_cert_console_init(void);
void              bospectra_cert_console_shutdown(void);

bospectra_error_t bospectra_cert_console_dispatch(const char* cmd);

#endif /* BOSPECTRA_V3_CERT_CONSOLE_H */
