#include "include/bospectra_color.h"
#include "pipeline/color_pipeline.h"
#include "diagnostics/color_diag.h"
#include "tests/color_tests.h"
#include "../debug/bospectra_debug.h"
#include "../include/bospectra_errors.h"

static bool g_color_engine_initialized = false;

bospectra_error_t BOSPECTRA_Color_Init(void) {
    if (g_color_engine_initialized) {
        return BOSPECTRA_ERR_ALREADY_INITIALIZED;
    }

    bospectra_color_diag_init();

    g_color_engine_initialized = true;
    bospectra_log("COLOR_ENGINE", "Color Engine Subsystem Initialized (BT.601 & BT.709 Matrices Loaded).");

    // Run automated color accuracy & reference pattern tests (disabled for instant boot)
    // bospectra_color_tests_run_all();

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Color_Shutdown(void) {
    if (!g_color_engine_initialized) {
        return BOSPECTRA_ERR_NOT_INITIALIZED;
    }

    bospectra_color_diag_shutdown();
    g_color_engine_initialized = false;

    return BOSPECTRA_SUCCESS;
}

bospectra_error_t BOSPECTRA_Color_ConvertFrame(const BOSFrame* src_frame, bospectra_pixel_format_t target_format, bospectra_color_space_t color_space, BOSFrame** out_converted_frame) {
    if (!g_color_engine_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;

    bospectra_trace_str("TRACE 10 — Color Engine", "Color Space Conversion");
    bospectra_trace_str("Input Format", "YUV420P");
    bospectra_trace_str("Output Format", "ARGB32");
    if (src_frame) {
        bospectra_trace_u32("Width", src_frame->width);
        bospectra_trace_u32("Height", src_frame->height);
    }

    bospectra_error_t err = bospectra_color_pipeline_process(src_frame, target_format, color_space, out_converted_frame);
    bool success = (err == BOSPECTRA_SUCCESS);

    if (success && out_converted_frame && *out_converted_frame) {
        bospectra_trace_hex("ARGB32 Frame Address", (uint64_t)(uintptr_t)(*out_converted_frame)->data[0]);
        bospectra_trace_str("Conversion Success", "TRUE");
    } else {
        bospectra_trace_str("Conversion Success", "FAILED");
    }

    uint32_t pixels = (src_frame && src_frame->width > 0) ? (src_frame->width * src_frame->height) : 0;
    bospectra_color_diag_record_conversion(10, pixels, success); // Microsecond telemetry record

    return err;
}

void BOSPECTRA_Color_GetStats(BOSPECTRA_ColorStats* out_stats) {
    bospectra_color_collect_stats(out_stats);
}

void BOSPECTRA_Color_DumpDiagnostics(void) {
    bospectra_color_dump_telemetry();
}
