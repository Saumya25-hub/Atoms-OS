/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_debug.c — Debug & Trace Logging Implementation
 */

#include "kernel/ipc/debug/ipc_debug.h"
#include "kernel/drivers/display/display.h"

static ipc_log_level_t g_ipc_log_level = IPC_LOG_INFO;

void ipc_debug_init(void) {
    g_ipc_log_level = IPC_LOG_INFO;
}

void ipc_debug_set_level(ipc_log_level_t level) {
    g_ipc_log_level = level;
}

void ipc_debug_log(ipc_log_level_t level, const char* subsystem, const char* message) {
    if (level > g_ipc_log_level) return;
    if (!subsystem || !message) return;

    display_print("[IPC_DEBUG] [");
    display_print(subsystem);
    display_print("] ");
    display_print(message);
    display_print("\n");
}

void ipc_debug_log_hex(ipc_log_level_t level, const char* subsystem,
                        const char* prefix, uint64_t value) {
    if (level > g_ipc_log_level) return;
    if (!subsystem || !prefix) return;

    display_print("[IPC_DEBUG] [");
    display_print(subsystem);
    display_print("] ");
    display_print(prefix);
    display_print("0x");
    display_print_hex(value);
    display_print("\n");
}
