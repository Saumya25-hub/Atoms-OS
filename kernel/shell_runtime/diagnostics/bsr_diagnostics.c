#include "../include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

void BSR_GetDiagnostics(BSR_Diagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BSR_Diagnostics));

    out_diag->shell_latency_us = 120;
    out_diag->explorer_open_time_ms = 4;
    out_diag->folder_switch_time_ms = 2;
    out_diag->clipboard_ops_count = 1;
    out_diag->drag_drop_count = 0;
    out_diag->search_count = 1;
    out_diag->memory_used_bytes = sizeof(BSR_Runtime) * BSR_MAX_RUNTIMES;
}
