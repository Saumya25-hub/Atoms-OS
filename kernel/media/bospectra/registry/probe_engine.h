/*
 * BOSPECTRA V3 — Dynamic Probe Engine
 * kernel/media/bospectra/registry/probe_engine.h
 *
 * Automated header byte probing, scoring engine (0-100), and driver selection.
 */

#ifndef BOSPECTRA_V3_PROBE_ENGINE_H
#define BOSPECTRA_V3_PROBE_ENGINE_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../file/bospectra_file.h"
#include "container_registry.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const BOSPECTRA_ContainerDriverRecord* selected_driver;
    int highest_score;
    uint32_t probed_bytes;
} BOSPECTRA_ProbeResult;

void bospectra_v3_probe_engine_init(void);
void bospectra_v3_probe_engine_shutdown(void);

bospectra_error_t bospectra_v3_probe_file(bospectra_file_id_t file_id, BOSPECTRA_ProbeResult* out_result);

#endif /* BOSPECTRA_V3_PROBE_ENGINE_H */
