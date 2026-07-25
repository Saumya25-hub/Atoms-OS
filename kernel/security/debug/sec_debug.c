/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_debug.c — Isolated Security Debugging & Tracing Engine Implementation
 */

#include "kernel/security/debug/sec_debug.h"
#include <stdarg.h>

extern void display_print(const char* str);

static uint32_t g_trace_mask = BOS_SEC_TRACE_NONE;

void sec_debug_init(void) {
    g_trace_mask = BOS_SEC_TRACE_NONE;
}

void sec_debug_set_trace_mask(uint32_t mask) {
    g_trace_mask = mask;
}

uint32_t sec_debug_get_trace_mask(void) {
    return g_trace_mask;
}

void sec_debug_log(uint32_t flag, const char* module, const char* fmt, ...) {
    if ((g_trace_mask & flag) == 0) return;

    display_print("[SEC_TRACE:");
    display_print(module ? module : "CORE");
    display_print("] ");
    display_print(fmt);
    display_print("\n");
}
