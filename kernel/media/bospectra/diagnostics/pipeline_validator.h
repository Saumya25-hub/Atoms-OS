/*
 * BOSPECTRA V3 — Production Pipeline Validator Subsystem
 * kernel/media/bospectra/diagnostics/pipeline_validator.h
 *
 * Full system health audit validating 28+ multimedia kernel subsystems.
 */

#ifndef BOSPECTRA_V3_PIPELINE_VALIDATOR_H
#define BOSPECTRA_V3_PIPELINE_VALIDATOR_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t subsystems_passed;
    uint32_t subsystems_failed;
    uint32_t warnings;
    uint32_t critical_errors;
    bool     playback_ready;
} BOSPECTRA_PipelineValidationResult;

void                               bospectra_pipeline_validator_init(void);
void                               bospectra_pipeline_validator_shutdown(void);

BOSPECTRA_PipelineValidationResult bospectra_pipeline_validate(void);

#endif /* BOSPECTRA_V3_PIPELINE_VALIDATOR_H */
