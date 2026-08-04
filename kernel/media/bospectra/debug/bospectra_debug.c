#include "bospectra_debug.h"
#include "bospectra_version.h"
#include "../core/bospectra_state.h"
#include "../include/bospectra_errors.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bool g_bospectra_debug_initialized = false;

void bospectra_debug_init(void) {
    g_bospectra_debug_initialized = true;
}

void bospectra_debug_shutdown(void) {
    g_bospectra_debug_initialized = false;
}

void bospectra_log(const char* level, const char* message) {
    if (!message) return;
    display_print("[BOSPECTRA:");
    if (level) display_print(level);
    else display_print("INFO");
    display_print("] ");
    display_print(message);
    display_print("\n");
}

bospectra_error_t bospectra_debug_get_diagnostics(BOSPECTRA_Diagnostics* out_diag) {
    if (!out_diag) return BOSPECTRA_ERR_INVALID_ARGUMENT;

    memset(out_diag, 0, sizeof(BOSPECTRA_Diagnostics));
    out_diag->engine_state = bospectra_state_get();
    
    out_diag->version.major = BOSPECTRA_VERSION_MAJOR;
    out_diag->version.minor = BOSPECTRA_VERSION_MINOR;
    out_diag->version.patch = BOSPECTRA_VERSION_PATCH;
    out_diag->version.build = BOSPECTRA_VERSION_BUILD;
    strncpy(out_diag->version.build_tag, BOSPECTRA_VERSION_TAG, sizeof(out_diag->version.build_tag) - 1);

    bospectra_memory_get_stats(&out_diag->memory_stats);

    out_diag->core_diag.subsystem_name = "Core";
    out_diag->core_diag.is_initialized = g_bospectra_debug_initialized;
    out_diag->core_diag.is_healthy = (out_diag->engine_state != BOSPECTRA_STATE_ERROR);

    out_diag->memory_diag.subsystem_name = "Memory";
    out_diag->memory_diag.is_initialized = true;
    out_diag->memory_diag.is_healthy = true;
    out_diag->memory_diag.allocated_bytes = out_diag->memory_stats.total_allocated_bytes;

    return BOSPECTRA_SUCCESS;
}

void bospectra_debug_dump(void) {
    BOSPECTRA_Diagnostics diag;
    if (bospectra_debug_get_diagnostics(&diag) != BOSPECTRA_SUCCESS) {
        display_print("[BOSPECTRA:ERROR] Failed to collect diagnostics!\n");
        return;
    }

    display_print("=============== BOSPECTRA ENGINE DIAGNOSTICS ===============\n");
    display_print("Version:      "); display_print(BOSPECTRA_VERSION_STR); display_print("\n");
    display_print("State:        "); display_print(bospectra_state_to_string(diag.engine_state)); display_print("\n");
    display_print("Core Health:  "); display_print(diag.core_diag.is_healthy ? "OK" : "ERROR"); display_print("\n");
    display_print("Memory Stats: Live Allocations = ");
    // Print memory info
    display_print(diag.memory_stats.active_allocation_count > 0 ? "Active" : "Clean");
    display_print("\n===========================================================\n");
}

void bospectra_trace_str(const char* label, const char* value) {
    display_print("[BOSPECTRA:TRACE] ");
    if (label) display_print(label);
    display_print(": ");
    if (value) display_print(value);
    else display_print("(null)");
    display_print("\n");
}

void bospectra_trace_u32(const char* label, uint32_t value) {
    char buf[32];
    char tmp[32];
    int idx = 0;
    if (value == 0) {
        buf[0] = '0'; buf[1] = '\0';
    } else {
        uint32_t v = value;
        while (v > 0) {
            tmp[idx++] = '0' + (v % 10);
            v /= 10;
        }
        for (int i = 0; i < idx; i++) {
            buf[i] = tmp[idx - 1 - i];
        }
        buf[idx] = '\0';
    }
    bospectra_trace_str(label, buf);
}

void bospectra_trace_hex(const char* label, uint64_t value) {
    char buf[32];
    buf[0] = '0'; buf[1] = 'x';
    const char* hex_chars = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        uint8_t nibble = (value >> ((15 - i) * 4)) & 0xF;
        buf[2 + i] = hex_chars[nibble];
    }
    buf[18] = '\0';
    bospectra_trace_str(label, buf);
}

void bospectra_trace_error(const char* func, bospectra_error_t err, const char* file, int line, const char* reason) {
    display_print("[BOSPECTRA:TRACE] *** ERROR IN ");
    if (func) display_print(func);
    display_print(" *** Code=");
    bospectra_trace_u32("ErrCode", (uint32_t)err);
    if (file) { display_print(" File="); display_print(file); }
    if (reason) { display_print(" Reason="); display_print(reason); }
    display_print("\n");
}
