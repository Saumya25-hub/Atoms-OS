#ifndef RENDER_DIAG_H
#define RENDER_DIAG_H

#include "../include/bospectra_render.h"

void bospectra_render_diag_init(void);
void bospectra_render_diag_shutdown(void);
void bospectra_render_diag_record_frame(uint64_t render_time_us, const char* backend_name, bool success);
void bospectra_render_collect_stats(BOSPECTRA_RenderStats* out_stats);
void bospectra_render_dump_telemetry(void);

#endif // RENDER_DIAG_H
