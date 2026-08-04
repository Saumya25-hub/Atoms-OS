#ifndef COLOR_DIAG_H
#define COLOR_DIAG_H

#include "../include/bospectra_color.h"

void bospectra_color_diag_init(void);
void bospectra_color_diag_shutdown(void);
void bospectra_color_diag_record_conversion(uint64_t conversion_time_us, uint32_t pixel_count, bool success);
void bospectra_color_collect_stats(BOSPECTRA_ColorStats* out_stats);
void bospectra_color_dump_telemetry(void);

#endif // COLOR_DIAG_H
