/*
 * BOSPECTRA V3 — Memory Validator Subsystem
 * kernel/media/bospectra/diagnostics/memory_validator.h
 */

#ifndef BOSPECTRA_V3_MEMORY_VALIDATOR_H
#define BOSPECTRA_V3_MEMORY_VALIDATOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void bospectra_memory_validator_init(void);
void bospectra_memory_validator_shutdown(void);

void bospectra_memory_validator_dump_leaks(void);

#endif /* BOSPECTRA_V3_MEMORY_VALIDATOR_H */
