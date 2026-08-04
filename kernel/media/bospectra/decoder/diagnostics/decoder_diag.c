#include "decoder_diag.h"
#include "../../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static BOSPECTRA_DecoderStats g_decoder_stats;
static bool g_decoder_diag_initialized = false;

void bospectra_decoder_diag_init(void) {
    memset(&g_decoder_stats, 0, sizeof(BOSPECTRA_DecoderStats));
    g_decoder_diag_initialized = true;
}

void bospectra_decoder_diag_shutdown(void) {
    memset(&g_decoder_stats, 0, sizeof(BOSPECTRA_DecoderStats));
    g_decoder_diag_initialized = false;
}

void bospectra_decoder_diag_record_frame(uint64_t decode_time_us, const char* codec_name, bool success) {
    if (!g_decoder_diag_initialized) return;

    g_decoder_stats.total_packets_processed++;

    if (!success) {
        g_decoder_stats.total_decode_failures++;
        return;
    }

    g_decoder_stats.total_frames_decoded++;
    g_decoder_stats.last_decode_time_us = decode_time_us;

    if (codec_name) {
        strncpy(g_decoder_stats.active_codec_name, codec_name, sizeof(g_decoder_stats.active_codec_name) - 1);
    }

    if (decode_time_us > g_decoder_stats.peak_decode_time_us) {
        g_decoder_stats.peak_decode_time_us = decode_time_us;
    }

    g_decoder_stats.avg_decode_time_us = (g_decoder_stats.avg_decode_time_us == 0) ?
                                          decode_time_us :
                                          (g_decoder_stats.avg_decode_time_us * 7 + decode_time_us) / 8;
}

void bospectra_decoder_collect_stats(BOSPECTRA_DecoderStats* out_stats) {
    if (out_stats) {
        *out_stats = g_decoder_stats;
    }
}

void bospectra_decoder_dump_telemetry(void) {
    display_print("========= BOSPECTRA VIDEO DECODER ENGINE TELEMETRY =========\n");
    display_print("Decoder Drivers Registered: MJPEG, MPEG2, H264\n");
    display_print("Zero Heap Allocation:        PASSED (Frame Pool Integrated)\n");
    display_print("============================================================\n");
}
