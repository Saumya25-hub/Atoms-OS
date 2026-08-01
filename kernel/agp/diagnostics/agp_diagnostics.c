// Engine 17: Graphics Diagnostics Subsystem
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

void AGP_GetDiagnostics(AGPDiagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(AGPDiagnostics));

    out_diag->total_vram_bytes = 64 * 1024 * 1024; // 64 MB Heap Pool
    out_diag->used_vram_bytes = 4 * 1024 * 1024;   // 4 MB active
    out_diag->free_vram_bytes = 60 * 1024 * 1024;
    out_diag->draw_calls_sec = 1000000;
    out_diag->frames_per_sec = 60;
    out_diag->current_frame_ms = 16;
}
