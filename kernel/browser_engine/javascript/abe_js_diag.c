#include "abe_js_diag.h"
#include "../diagnostics/abe_diagnostics.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool g_js_diag_initialized = false;

ABE_Error ABE_JSDiag_Init(void) {
    g_js_diag_initialized = true;
    return ABE_SUCCESS;
}

ABE_Error ABE_JSDiag_Shutdown(void) {
    g_js_diag_initialized = false;
    return ABE_SUCCESS;
}

void ABE_JSDiag_DumpVMStats(ABE_JSContextHandle ctx) {
    if (!g_js_diag_initialized) return;

    ABE_DiagnosticsMetrics metrics = ABE_Diagnostics_GetMetrics();
    display_print("=== ABE ECMAScript Runtime Diagnostic Audit ===\n");
    display_print(" Contexts Created        : "); display_print_dec(metrics.js_contexts_created); display_print("\n");
    display_print(" Scripts Executed        : "); display_print_dec(metrics.js_scripts_executed); display_print("\n");
    display_print(" Instructions Executed   : "); display_print_dec(metrics.js_bytecode_instructions_executed); display_print("\n");
    display_print(" GC Runs Total           : "); display_print_dec(metrics.js_gc_runs_total); display_print("\n");
    display_print(" Promises Resolved       : "); display_print_dec(metrics.js_promises_resolved); display_print("\n");
    display_print(" Event Loop Ticks        : "); display_print_dec(metrics.js_event_loop_ticks); display_print("\n");
    display_print(" Execution Time (us)     : "); display_print_dec(metrics.js_execution_time_us); display_print("\n");
    display_print("================================================\n");
}
