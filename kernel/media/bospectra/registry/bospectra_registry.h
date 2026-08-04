/*
 * BOSPECTRA V3 — Dynamic Multimedia Registry System
 * kernel/media/bospectra/registry/bospectra_registry.h
 *
 * Top-level master header for the Dynamic Multimedia Registry Subsystem.
 */

#ifndef BOSPECTRA_V3_REGISTRY_H
#define BOSPECTRA_V3_REGISTRY_H

#include "driver_registry.h"
#include "container_registry.h"
#include "codec_registry.h"
#include "renderer_registry.h"
#include "probe_engine.h"

bospectra_error_t bospectra_v3_registry_init(void);
bospectra_error_t bospectra_v3_registry_shutdown(void);
void bospectra_v3_registry_dump_diagnostics(void);

#endif /* BOSPECTRA_V3_REGISTRY_H */
