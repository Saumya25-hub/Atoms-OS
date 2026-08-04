#include "include/bospectra_container.h"
#include "include/bospectra_demux.h"
#include "registry/container_registry.h"
#include "mp4/mp4_parser.h"
#include "avi/avi_parser.h"
#include "mkv/mkv_parser.h"
#include "tests/container_tests.h"
#include "../memory/bospectra_memory.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_DEMUX_SESSIONS 8U

typedef struct {
    bospectra_demux_id_t             id;
    bospectra_file_id_t              file_id;
    const BOSPECTRA_ContainerDriver* driver;
    void*                            driver_ctx;
    BOSPECTRA_ContainerMetadata      metadata;
    bool                             is_active;
} DemuxSession;

static DemuxSession g_demux_sessions[BOSPECTRA_MAX_DEMUX_SESSIONS];
static bool g_container_engine_initialized = false;

bospectra_error_t BOSPECTRA_Container_Init(void) {
    if (g_container_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    memset(g_demux_sessions, 0, sizeof(g_demux_sessions));
    bospectra_container_registry_init();

    // Register builtin parsers
    bospectra_container_register_driver(&g_mp4_container_driver);
    bospectra_container_register_driver(&g_avi_container_driver);
    bospectra_container_register_driver(&g_mkv_container_driver);

    g_container_engine_initialized = true;
    bospectra_log("CONTAINER", "Container Engine Subsystem Initialized (MP4, AVI, MKV Drivers Loaded).");

    // Run container self-tests (disabled for instant boot)
    // bospectra_container_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Container_Shutdown(void) {
    if (!g_container_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    for (uint32_t i = 0; i < BOSPECTRA_MAX_DEMUX_SESSIONS; i++) {
        if (g_demux_sessions[i].is_active) {
            BOSPECTRA_Demux_Close(g_demux_sessions[i].id);
        }
    }

    bospectra_container_registry_shutdown();
    memset(g_demux_sessions, 0, sizeof(g_demux_sessions));
    g_container_engine_initialized = false;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Container_DetectFormat(const char* file_path, char* out_format_name, size_t max_name_len) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!file_path || !out_format_name) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    bospectra_file_id_t fid = 0;
    if (bospectra_file_open(file_path, &fid) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_FILE_NOT_FOUND;
    }

    const BOSPECTRA_ContainerDriver* drv = bospectra_container_probe_driver(fid);
    bospectra_file_close(fid);

    if (!drv) return BOSPECTRA_ERR_FILE_READ_FAILED;

    strncpy(out_format_name, drv->format_name, max_name_len - 1);
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Demux_Open(const char* file_path, bospectra_demux_id_t* out_demux_id) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!file_path || !out_demux_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t slot = BOSPECTRA_MAX_DEMUX_SESSIONS;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_DEMUX_SESSIONS; i++) {
        if (!g_demux_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot >= BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    bospectra_file_id_t fid = 0;
    if (bospectra_file_open(file_path, &fid) != BOSPECTRA_SUCCESS) {
        return BOSPECTRA_ERR_FILE_NOT_FOUND;
    }

    const BOSPECTRA_ContainerDriver* drv = bospectra_container_probe_driver(fid);
    if (!drv) {
        bospectra_file_close(fid);
        return BOSPECTRA_ERR_FILE_READ_FAILED;
    }

    void* drv_ctx = NULL;
    BOSPECTRA_ContainerMetadata meta;
    memset(&meta, 0, sizeof(BOSPECTRA_ContainerMetadata));

    bospectra_error_t err = drv->open(&drv_ctx, fid, &meta);
    if (err != BOSPECTRA_SUCCESS) {
        bospectra_file_close(fid);
        return err;
    }

    DemuxSession* sess = &g_demux_sessions[slot];
    sess->id = slot + 1;
    sess->file_id = fid;
    sess->driver = drv;
    sess->driver_ctx = drv_ctx;
    sess->metadata = meta;
    sess->is_active = true;

    *out_demux_id = sess->id;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Demux_GetMetadata(bospectra_demux_id_t demux_id, BOSPECTRA_ContainerMetadata* out_meta) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_meta) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (demux_id == 0 || demux_id > BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = demux_id - 1;
    if (!g_demux_sessions[idx].is_active) return BOSPECTRA_ERR_HANDLE_INVALID;

    *out_meta = g_demux_sessions[idx].metadata;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Demux_GetStreamDescriptor(bospectra_demux_id_t demux_id, uint32_t stream_index, BOSPECTRA_StreamDescriptor* out_desc) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_desc) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (demux_id == 0 || demux_id > BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = demux_id - 1;
    DemuxSession* sess = &g_demux_sessions[idx];
    if (!sess->is_active || !sess->driver || !sess->driver->get_stream) return BOSPECTRA_ERR_HANDLE_INVALID;

    return sess->driver->get_stream(sess->driver_ctx, stream_index, out_desc);
}

bospectra_error_t BOSPECTRA_Demux_ReadPacket(bospectra_demux_id_t demux_id, BOSPacket** out_packet) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!out_packet) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (demux_id == 0 || demux_id > BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = demux_id - 1;
    DemuxSession* sess = &g_demux_sessions[idx];
    if (!sess->is_active || !sess->driver || !sess->driver->read_packet) return BOSPECTRA_ERR_HANDLE_INVALID;

    return sess->driver->read_packet(sess->driver_ctx, out_packet);
}

bospectra_error_t BOSPECTRA_Demux_Seek(bospectra_demux_id_t demux_id, uint64_t timestamp_us) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (demux_id == 0 || demux_id > BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = demux_id - 1;
    DemuxSession* sess = &g_demux_sessions[idx];
    if (!sess->is_active || !sess->driver || !sess->driver->seek) return BOSPECTRA_ERR_HANDLE_INVALID;

    return sess->driver->seek(sess->driver_ctx, timestamp_us);
}

bospectra_error_t BOSPECTRA_Demux_Close(bospectra_demux_id_t demux_id) {
    if (!g_container_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (demux_id == 0 || demux_id > BOSPECTRA_MAX_DEMUX_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = demux_id - 1;
    DemuxSession* sess = &g_demux_sessions[idx];
    if (!sess->is_active) return BOSPECTRA_ERR_HANDLE_INVALID;

    if (sess->driver && sess->driver->close) {
        sess->driver->close(sess->driver_ctx);
    }
    bospectra_file_close(sess->file_id);

    memset(sess, 0, sizeof(DemuxSession));
    return BOSPECTRA_SUCCESS;
}
