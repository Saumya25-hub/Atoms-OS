/*
 * BOSPECTRA V3 — Central Master Driver Registry Implementation
 * kernel/media/bospectra/registry/driver_registry.c
 */

#include "driver_registry.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_master_driver_registry_initialized = false;

void bospectra_v3_driver_registry_init(void) {
    bospectra_v3_container_registry_init();
    bospectra_v3_codec_registry_init();
    bospectra_v3_renderer_registry_init();
    bospectra_v3_probe_engine_init();

    g_master_driver_registry_initialized = true;
    bospectra_log("MASTER_DRIVER_REGISTRY", "BOSPECTRA V3 Central Driver Registry Initialized.");
}

void bospectra_v3_driver_registry_shutdown(void) {
    bospectra_v3_probe_engine_shutdown();
    bospectra_v3_renderer_registry_shutdown();
    bospectra_v3_codec_registry_shutdown();
    bospectra_v3_container_registry_shutdown();
    g_master_driver_registry_initialized = false;
}

void bospectra_v3_driver_registry_dump_telemetry(void) {
    if (!g_master_driver_registry_initialized) return;

    display_print("\n============= BOSPECTRA REGISTRY =============");
    display_print("\nContainers Registered:");

    const BOSPECTRA_ContainerDriverRecord* containers[16];
    uint32_t c_count = bospectra_v3_container_get_registered(containers, 16);
    for (uint32_t i = 0; i < c_count; i++) {
        display_print("\n  - ");
        display_print(containers[i]->format_name);
        display_print(" (Ext: ");
        display_print(containers[i]->extensions);
        display_print(")");
    }

    display_print("\n\nCodecs Registered:");
    const BOSPECTRA_CodecDriverRecord* codecs[16];
    uint32_t cd_count = bospectra_v3_codec_get_registered(codecs, 16);
    for (uint32_t i = 0; i < cd_count; i++) {
        display_print("\n  - ");
        display_print(codecs[i]->codec_name);
    }

    display_print("\n\nRenderers Registered:");
    const BOSPECTRA_RendererRecord* renderers[8];
    uint32_t r_count = bospectra_v3_renderer_get_registered(renderers, 8);
    for (uint32_t i = 0; i < r_count; i++) {
        display_print("\n  - ");
        display_print(renderers[i]->backend_name);
    }

    display_print("\n==============================================\n\n");
}
