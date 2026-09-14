/*
 * BOSPECTRA V3 — Decoder Manager Implementation
 * kernel/media/bospectra/manager/decoder_manager.c
 */

#include "decoder_manager.h"
#include "../decoder/mjpeg/mjpeg_decoder.h"
#include "../decoder/h264/h264_decoder.h"
#include "../decoder/mpeg2/mpeg2_decoder.h"
#include "../decoder/hevc/hevc_decoder.h"
#include "../decoder/vp8/vp8_decoder.h"
#include "../decoder/vp9/vp9_decoder.h"
#include "../decoder/include/video_accel.h"
#include "../registry/codec_registry.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern const BOSPECTRA_DecoderDriver g_mjpeg_decoder_driver;
extern const BOSPECTRA_DecoderDriver g_h264_decoder_driver;
extern const BOSPECTRA_DecoderDriver g_mpeg2_decoder_driver;
extern const BOSPECTRA_DecoderDriver g_hevc_decoder_driver;
extern const BOSPECTRA_DecoderDriver g_vp8_decoder_driver;
extern const BOSPECTRA_DecoderDriver g_vp9_decoder_driver;

static bool g_decoder_manager_initialized = false;

void bospectra_decoder_manager_init(void) {
    bospectra_decoder_registry_init();
    bospectra_v3_codec_registry_init();

    /* Initialize Video Acceleration HAL */
    bospectra_video_accel_init();

    /* Register built-in decoder drivers in Legacy & V3 Codec Registries */
    bospectra_decoder_register_driver(&g_mjpeg_decoder_driver);
    bospectra_decoder_register_driver(&g_h264_decoder_driver);
    bospectra_decoder_register_driver(&g_mpeg2_decoder_driver);
    bospectra_decoder_register_driver(&g_hevc_decoder_driver);
    bospectra_decoder_register_driver(&g_vp8_decoder_driver);
    bospectra_decoder_register_driver(&g_vp9_decoder_driver);

    BOSPECTRA_CodecDriverRecord rec_mjpeg = {
        .codec_id = BOSPECTRA_CODEC_MJPEG, .codec_name = "MJPEG", .priority = 90,
        .open = g_mjpeg_decoder_driver.open, .decode_packet = g_mjpeg_decoder_driver.decode_packet,
        .flush = g_mjpeg_decoder_driver.flush, .close = g_mjpeg_decoder_driver.close
    };
    BOSPECTRA_CodecDriverRecord rec_h264 = {
        .codec_id = BOSPECTRA_CODEC_H264, .codec_name = "H264", .priority = 95,
        .open = g_h264_decoder_driver.open, .decode_packet = g_h264_decoder_driver.decode_packet,
        .flush = g_h264_decoder_driver.flush, .close = g_h264_decoder_driver.close
    };
    BOSPECTRA_CodecDriverRecord rec_mpeg2 = {
        .codec_id = BOSPECTRA_CODEC_MPEG2, .codec_name = "MPEG2", .priority = 80,
        .open = g_mpeg2_decoder_driver.open, .decode_packet = g_mpeg2_decoder_driver.decode_packet,
        .flush = g_mpeg2_decoder_driver.flush, .close = g_mpeg2_decoder_driver.close
    };
    BOSPECTRA_CodecDriverRecord rec_hevc = {
        .codec_id = BOSPECTRA_CODEC_HEVC, .codec_name = "HEVC", .priority = 95,
        .open = g_hevc_decoder_driver.open, .decode_packet = g_hevc_decoder_driver.decode_packet,
        .flush = g_hevc_decoder_driver.flush, .close = g_hevc_decoder_driver.close
    };
    BOSPECTRA_CodecDriverRecord rec_vp8 = {
        .codec_id = BOSPECTRA_CODEC_VP8, .codec_name = "VP8", .priority = 90,
        .open = g_vp8_decoder_driver.open, .decode_packet = g_vp8_decoder_driver.decode_packet,
        .flush = g_vp8_decoder_driver.flush, .close = g_vp8_decoder_driver.close
    };
    BOSPECTRA_CodecDriverRecord rec_vp9 = {
        .codec_id = BOSPECTRA_CODEC_VP9, .codec_name = "VP9", .priority = 95,
        .open = g_vp9_decoder_driver.open, .decode_packet = g_vp9_decoder_driver.decode_packet,
        .flush = g_vp9_decoder_driver.flush, .close = g_vp9_decoder_driver.close
    };

    bospectra_v3_codec_register(&rec_mjpeg);
    bospectra_v3_codec_register(&rec_h264);
    bospectra_v3_codec_register(&rec_mpeg2);
    bospectra_v3_codec_register(&rec_hevc);
    bospectra_v3_codec_register(&rec_vp8);
    bospectra_v3_codec_register(&rec_vp9);

    g_decoder_manager_initialized = true;
    bospectra_log("DECODER_MANAGER", "BOSPECTRA V3 Decoder Manager initialized (MJPEG, H.264, MPEG2, HEVC, VP8, VP9 Registered).");
}

void bospectra_decoder_manager_shutdown(void) {
    bospectra_v3_codec_registry_shutdown();
    bospectra_decoder_registry_shutdown();
    g_decoder_manager_initialized = false;
}

const BOSPECTRA_DecoderDriver* bospectra_decoder_resolve(const BOSPECTRA_StreamDescriptor* stream_desc) {
    if (!g_decoder_manager_initialized || !stream_desc) return NULL;

    const BOSPECTRA_CodecDriverRecord* rec = bospectra_v3_codec_find_by_name(stream_desc->codec_name);
    if (rec) {
        const BOSPECTRA_DecoderDriver* drv = bospectra_decoder_find_driver_by_name(rec->codec_name);
        if (drv) return drv;
    }

    const BOSPECTRA_DecoderDriver* drv = bospectra_decoder_find_driver_by_name(stream_desc->codec_name);
    if (!drv) {
        if (strcmp(stream_desc->codec_name, "avc1") == 0 || strcmp(stream_desc->codec_name, "h264") == 0 || strcmp(stream_desc->codec_name, "H264") == 0) {
            drv = &g_h264_decoder_driver;
        } else if (strcmp(stream_desc->codec_name, "hevc") == 0 || strcmp(stream_desc->codec_name, "hvc1") == 0 || strcmp(stream_desc->codec_name, "hev1") == 0 || strcmp(stream_desc->codec_name, "h265") == 0 || strcmp(stream_desc->codec_name, "HEVC") == 0) {
            drv = &g_hevc_decoder_driver;
        } else if (strcmp(stream_desc->codec_name, "vp8") == 0 || strcmp(stream_desc->codec_name, "vp08") == 0 || strcmp(stream_desc->codec_name, "VP8") == 0) {
            drv = &g_vp8_decoder_driver;
        } else if (strcmp(stream_desc->codec_name, "vp9") == 0 || strcmp(stream_desc->codec_name, "vp09") == 0 || strcmp(stream_desc->codec_name, "VP9") == 0) {
            drv = &g_vp9_decoder_driver;
        } else if (strcmp(stream_desc->codec_name, "mpeg2") == 0) {
            drv = &g_mpeg2_decoder_driver;
        } else {
            drv = &g_mjpeg_decoder_driver;
        }
    }
    return drv;
}
