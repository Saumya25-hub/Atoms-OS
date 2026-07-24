#include "abe_web_diag.h"
#include "../diagnostics/abe_diagnostics.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool g_web_diag_initialized = false;

ABE_Error ABE_WebDiag_Init(void) {
    g_web_diag_initialized = true;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebDiag_Shutdown(void) {
    g_web_diag_initialized = false;
    return ABE_SUCCESS;
}

void ABE_WebDiag_DumpStats(void) {
    if (!g_web_diag_initialized) return;

    ABE_DiagnosticsMetrics metrics = ABE_Diagnostics_GetMetrics();
    display_print("=== ABE Web Platform Diagnostics Audit ===\n");
    display_print(" Fetch Requests Total   : "); display_print_dec(metrics.fetch_requests_total); display_print("\n");
    display_print(" XHR Requests Total     : "); display_print_dec(metrics.xhr_requests_total); display_print("\n");
    display_print(" Storage Read Ops       : "); display_print_dec(metrics.storage_read_ops); display_print("\n");
    display_print(" Storage Write Ops      : "); display_print_dec(metrics.storage_write_ops); display_print("\n");
    display_print(" Animation Frames Req   : "); display_print_dec(metrics.animation_frames_requested); display_print("\n");
    display_print(" Mutation Observer Trig : "); display_print_dec(metrics.mutation_observer_triggers); display_print("\n");
    display_print("==========================================\n");
}
