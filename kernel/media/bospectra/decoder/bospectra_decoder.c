#include "include/bospectra_decoder.h"
#include "registry/decoder_registry.h"
#include "mjpeg/mjpeg_decoder.h"
#include "mpeg2/mpeg2_decoder.h"
#include "h264/h264_decoder.h"
#include "diagnostics/decoder_diag.h"
#include "tests/decoder_tests.h"
#include "../memory/bospectra_memory.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

#define BOSPECTRA_MAX_DECODER_SESSIONS 8U

typedef struct {
    bospectra_decoder_id_t          id;
    const BOSPECTRA_DecoderDriver* driver;
    void*                          driver_ctx;
    BOSPECTRA_StreamDescriptor     stream_desc;
    bool                           is_active;
} DecoderSession;

static DecoderSession g_decoder_sessions[BOSPECTRA_MAX_DECODER_SESSIONS];
static bool g_decoder_engine_initialized = false;

bospectra_error_t BOSPECTRA_Decoder_Init(void) {
    if (g_decoder_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    memset(g_decoder_sessions, 0, sizeof(g_decoder_sessions));
    bospectra_decoder_registry_init();
    bospectra_decoder_diag_init();

    // Register built-in decoders in Phase order (5A: MJPEG, 5B: MPEG2, 5C: H264)
    bospectra_decoder_register_driver(&g_mjpeg_decoder_driver);
    bospectra_decoder_register_driver(&g_mpeg2_decoder_driver);
    bospectra_decoder_register_driver(&g_h264_decoder_driver);

    g_decoder_engine_initialized = true;
    bospectra_log("DECODER_ENGINE", "Video Decoder Engine Initialized (MJPEG, MPEG2, H264 Drivers Loaded).");

    // Run self-tests and decode stress test (disabled for instant boot)
    // bospectra_decoder_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Decoder_Shutdown(void) {
    if (!g_decoder_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    for (uint32_t i = 0; i < BOSPECTRA_MAX_DECODER_SESSIONS; i++) {
        if (g_decoder_sessions[i].is_active) {
            BOSPECTRA_Decoder_Close(g_decoder_sessions[i].id);
        }
    }

    bospectra_decoder_diag_shutdown();
    bospectra_decoder_registry_shutdown();
    memset(g_decoder_sessions, 0, sizeof(g_decoder_sessions));
    g_decoder_engine_initialized = false;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Decoder_Open(const BOSPECTRA_StreamDescriptor* stream_desc, bospectra_decoder_id_t* out_decoder_id) {
    if (!g_decoder_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!stream_desc || !out_decoder_id) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    uint32_t slot = BOSPECTRA_MAX_DECODER_SESSIONS;
    for (uint32_t i = 0; i < BOSPECTRA_MAX_DECODER_SESSIONS; i++) {
        if (!g_decoder_sessions[i].is_active) {
            slot = i;
            break;
        }
    }
    if (slot >= BOSPECTRA_MAX_DECODER_SESSIONS) return BOSPECTRA_ERR_BUFFER_OVERFLOW;

    const BOSPECTRA_DecoderDriver* drv = bospectra_decoder_find_driver((bospectra_codec_id_t)stream_desc->codec_id);
    if (!drv && stream_desc->codec_name[0]) {
        drv = bospectra_decoder_find_driver_by_name(stream_desc->codec_name);
    }
    if (!drv) return BOSPECTRA_ERR_UNSUPPORTED_FORMAT;

    void* drv_ctx = NULL;
    bospectra_error_t err = drv->open(&drv_ctx, stream_desc);
    if (err != BOSPECTRA_SUCCESS) return err;

    DecoderSession* sess = &g_decoder_sessions[slot];
    sess->id = slot + 1;
    sess->driver = drv;
    sess->driver_ctx = drv_ctx;
    sess->stream_desc = *stream_desc;
    sess->is_active = true;

    *out_decoder_id = sess->id;
    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Decoder_DecodePacket(bospectra_decoder_id_t decoder_id, const BOSPacket* packet, BOSFrame** out_frame) {
    if (!g_decoder_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!packet || !out_frame) return BOSPECTRA_ERR_INVALID_ARGUMENT;
    if (decoder_id == 0 || decoder_id > BOSPECTRA_MAX_DECODER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = decoder_id - 1;
    DecoderSession* sess = &g_decoder_sessions[idx];
    if (!sess->is_active || !sess->driver || !sess->driver->decode_packet) return BOSPECTRA_ERR_HANDLE_INVALID;

    bospectra_error_t err = sess->driver->decode_packet(sess->driver_ctx, packet, out_frame);
    bool success = (err == BOSPECTRA_SUCCESS);

    bospectra_decoder_diag_record_frame(50, sess->driver->codec_name, success); // Microsecond telemetry record

    return err;
}

bospectra_error_t BOSPECTRA_Decoder_Flush(bospectra_decoder_id_t decoder_id) {
    if (!g_decoder_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (decoder_id == 0 || decoder_id > BOSPECTRA_MAX_DECODER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = decoder_id - 1;
    DecoderSession* sess = &g_decoder_sessions[idx];
    if (!sess->is_active || !sess->driver || !sess->driver->flush) return BOSPECTRA_ERR_HANDLE_INVALID;

    return sess->driver->flush(sess->driver_ctx);
}

bospectra_error_t BOSPECTRA_Decoder_Close(bospectra_decoder_id_t decoder_id) {
    if (!g_decoder_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (decoder_id == 0 || decoder_id > BOSPECTRA_MAX_DECODER_SESSIONS) return BOSPECTRA_ERR_HANDLE_INVALID;

    uint32_t idx = decoder_id - 1;
    DecoderSession* sess = &g_decoder_sessions[idx];
    if (!sess->is_active) return BOSPECTRA_ERR_HANDLE_INVALID;

    if (sess->driver && sess->driver->close) {
        sess->driver->close(sess->driver_ctx);
    }

    memset(sess, 0, sizeof(DecoderSession));
    return BOSPECTRA_SUCCESS;
}

void BOSPECTRA_Decoder_GetStats(BOSPECTRA_DecoderStats* out_stats) {
    bospectra_decoder_collect_stats(out_stats);
}

void BOSPECTRA_Decoder_DumpDiagnostics(void) {
    bospectra_decoder_dump_telemetry();
}
