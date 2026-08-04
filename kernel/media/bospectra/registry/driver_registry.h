/*
 * BOSPECTRA V3 — Central Master Driver Registry
 * kernel/media/bospectra/registry/driver_registry.h
 *
 * Central master coordinator for containers, codecs, renderers, audio decoders, color converters, and subtitle parsers.
 */

#ifndef BOSPECTRA_V3_DRIVER_REGISTRY_H
#define BOSPECTRA_V3_DRIVER_REGISTRY_H

#include "container_registry.h"
#include "codec_registry.h"
#include "renderer_registry.h"
#include "probe_engine.h"

void bospectra_v3_driver_registry_init(void);
void bospectra_v3_driver_registry_shutdown(void);

void bospectra_v3_driver_registry_dump_telemetry(void);

#endif /* BOSPECTRA_V3_DRIVER_REGISTRY_H */
