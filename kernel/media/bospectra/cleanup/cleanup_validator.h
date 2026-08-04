/*
 * BOSPECTRA V3 — Cleanup Validator Subsystem
 * kernel/media/bospectra/cleanup/cleanup_validator.h
 */

#ifndef BOSPECTRA_V3_CLEANUP_VALIDATOR_H
#define BOSPECTRA_V3_CLEANUP_VALIDATOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bospectra_cleanup_validator_init(void);
void bospectra_cleanup_validator_shutdown(void);

bool bospectra_cleanup_validate_unlinked(uint32_t object_id);

#endif /* BOSPECTRA_V3_CLEANUP_VALIDATOR_H */
