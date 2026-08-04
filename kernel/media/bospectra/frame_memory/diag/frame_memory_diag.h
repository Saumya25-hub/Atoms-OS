#ifndef FRAME_MEMORY_DIAG_H
#define FRAME_MEMORY_DIAG_H

#include "../include/bospectra_frame_memory.h"

void bospectra_frame_memory_diag_init(void);
void bospectra_frame_memory_diag_shutdown(void);
void bospectra_frame_memory_collect_stats(BOSPECTRA_FrameMemoryStats* out_stats);
void bospectra_frame_memory_dump_telemetry(void);

#endif // FRAME_MEMORY_DIAG_H
