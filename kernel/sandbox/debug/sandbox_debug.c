/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_debug.c — Debug Engine Subsystem Implementation
 */

#include "kernel/sandbox/debug/sandbox_debug.h"
#include <stdarg.h>

__attribute__((weak)) void display_print(const char* str) {
    (void)str;
}


static uint64_t g_trace_mask = 0;

void sandbox_debug_init(void) {
    g_trace_mask = 0;
}

void bos_sandbox_debug_set_trace(uint64_t trace_mask, bool enable) {
    if (enable) {
        g_trace_mask |= trace_mask;
    } else {
        g_trace_mask &= ~trace_mask;
    }
}

uint64_t bos_sandbox_debug_get_trace_mask(void) {
    return g_trace_mask;
}

void bos_sandbox_debug_log(uint64_t trace_flag, const char *fmt, ...) {
    if ((g_trace_mask & trace_flag) == 0) return;

    display_print("[SANDBOX_TRACE] ");
    if (fmt) {
        display_print(fmt);
    }
    display_print("\n");
}
