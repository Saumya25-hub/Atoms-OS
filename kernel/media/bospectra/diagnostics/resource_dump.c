/*
 * BOSPECTRA V3 — Resource Dump Implementation
 * kernel/media/bospectra/diagnostics/resource_dump.c
 */

#include "resource_dump.h"
#include "../resource/resource_metrics.h"

void bospectra_dump_resources(void) {
    bospectra_resource_metrics_dump();
}
