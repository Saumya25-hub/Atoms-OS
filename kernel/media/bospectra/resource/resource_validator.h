/*
 * BOSPECTRA V3 — Resource Validator Subsystem
 * kernel/media/bospectra/resource/resource_validator.h
 *
 * Inspects ownership graph, reference counts, and lifetime states to catch violations.
 */

#ifndef BOSPECTRA_V3_RESOURCE_VALIDATOR_H
#define BOSPECTRA_V3_RESOURCE_VALIDATOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t total_inspections;
    uint32_t orphan_objects_detected;
    uint32_t double_frees_prevented;
    uint32_t stale_pointers_caught;
} BOSPECTRA_ValidatorMetrics;

void                      bospectra_resource_validator_init(void);
void                      bospectra_resource_validator_shutdown(void);

bospectra_error_t         bospectra_resource_validate_all(void);
BOSPECTRA_ValidatorMetrics bospectra_resource_validator_get_metrics(void);

#endif /* BOSPECTRA_V3_RESOURCE_VALIDATOR_H */
