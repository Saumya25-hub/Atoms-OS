/*
 * BOSPECTRA V3 — Container Manager Implementation
 * kernel/media/bospectra/manager/container_manager.c
 */

#include "container_manager.h"
#include "../container/avi/avi_parser.h"
#include "../container/mp4/mp4_parser.h"
#include "../container/mkv/mkv_parser.h"
#include "../registry/container_registry.h"
#include "../registry/probe_engine.h"
#include "../debug/bospectra_debug.h"

extern const BOSPECTRA_ContainerDriver g_avi_container_driver;
extern const BOSPECTRA_ContainerDriver g_mp4_container_driver;
extern const BOSPECTRA_ContainerDriver g_mkv_container_driver;

static bool g_container_manager_initialized = false;

void bospectra_container_manager_init(void) {
    bospectra_container_registry_init();
    bospectra_v3_container_registry_init();

    /* Automatically register built-in OS container drivers in Legacy & V3 Registries */
    bospectra_container_register_driver(&g_avi_container_driver);
    bospectra_container_register_driver(&g_mp4_container_driver);
    bospectra_container_register_driver(&g_mkv_container_driver);

    BOSPECTRA_ContainerDriverRecord rec_avi = {
        .format_name = "AVI", .extensions = "avi", .priority = 90,
        .probe = g_avi_container_driver.probe, .open = g_avi_container_driver.open,
        .get_stream = g_avi_container_driver.get_stream, .read_packet = g_avi_container_driver.read_packet,
        .seek = g_avi_container_driver.seek, .close = g_avi_container_driver.close
    };
    BOSPECTRA_ContainerDriverRecord rec_mp4 = {
        .format_name = "MP4", .extensions = "mp4,m4v,mov", .priority = 95,
        .probe = g_mp4_container_driver.probe, .open = g_mp4_container_driver.open,
        .get_stream = g_mp4_container_driver.get_stream, .read_packet = g_mp4_container_driver.read_packet,
        .seek = g_mp4_container_driver.seek, .close = g_mp4_container_driver.close
    };
    BOSPECTRA_ContainerDriverRecord rec_mkv = {
        .format_name = "MKV", .extensions = "mkv,webm", .priority = 85,
        .probe = g_mkv_container_driver.probe, .open = g_mkv_container_driver.open,
        .get_stream = g_mkv_container_driver.get_stream, .read_packet = g_mkv_container_driver.read_packet,
        .seek = g_mkv_container_driver.seek, .close = g_mkv_container_driver.close
    };

    bospectra_v3_container_register(&rec_avi);
    bospectra_v3_container_register(&rec_mp4);
    bospectra_v3_container_register(&rec_mkv);

    g_container_manager_initialized = true;
    bospectra_log("CONTAINER_MANAGER", "BOSPECTRA V3 Container Manager initialized (AVI, MP4, MKV Registered in V3 Engine).");
}

void bospectra_container_manager_shutdown(void) {
    bospectra_v3_container_registry_shutdown();
    bospectra_container_registry_shutdown();
    g_container_manager_initialized = false;
}

const BOSPECTRA_ContainerDriver* bospectra_container_auto_probe(bospectra_file_id_t file_id) {
    if (!g_container_manager_initialized) return NULL;

    BOSPECTRA_ProbeResult pres;
    if (bospectra_v3_probe_file(file_id, &pres) == BOSPECTRA_SUCCESS && pres.selected_driver) {
        /* Map back to vtable driver pointer */
        const BOSPECTRA_ContainerDriver* drv = bospectra_container_find_driver_by_name(pres.selected_driver->format_name);
        if (drv) return drv;
    }

    return bospectra_container_probe_driver(file_id);
}
