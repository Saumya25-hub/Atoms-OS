#ifndef BOSPECTRA_COLOR_H
#define BOSPECTRA_COLOR_H

#include "../../include/bospectra_types.h"
#include "../../frame_memory/include/bospectra_frame.h"
#include "bospectra_color_spaces.h"

typedef struct {
    uint64_t total_frames_converted;
    uint64_t total_pixels_converted;
    uint64_t last_conversion_time_us;
    uint64_t avg_conversion_time_us;
    uint64_t peak_conversion_time_us;
    uint64_t conversion_failures;
} BOSPECTRA_ColorStats;

// Master Color Engine Lifecycle APIs
bospectra_error_t BOSPECTRA_Color_Init(void);
bospectra_error_t BOSPECTRA_Color_Shutdown(void);

// Main Pixel Format Conversion Entry Point
bospectra_error_t BOSPECTRA_Color_ConvertFrame(const BOSFrame* src_frame, bospectra_pixel_format_t target_format, bospectra_color_space_t color_space, BOSFrame** out_converted_frame);

// Telemetry & Diagnostics Query
void BOSPECTRA_Color_GetStats(BOSPECTRA_ColorStats* out_stats);
void BOSPECTRA_Color_DumpDiagnostics(void);

#endif // BOSPECTRA_COLOR_H
