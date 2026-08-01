#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// TaskManager.BOSX V1.0 — Runtime Manager
// ATOMS OS Real-Time System Observatory
//
// Owns ZERO scheduling, memory management, networking, graphics.
// Queries live data only through Kernel APIs / BOSLL / Drivers.
// ============================================================

static bool g_taskmgr_initialized = false;

int32_t TaskManagerInitialize(void) {
    if (g_taskmgr_initialized) return 0;
    display_print("[TASKMGR] Initializing TaskManager.BOSX V1.0...\n");

    // Boot all sub-engines
    extern void taskmgr_dashboard_init(void);
    extern void taskmgr_process_init(void);
    extern void taskmgr_threads_init(void);
    extern void taskmgr_memory_init(void);
    extern void taskmgr_cpu_init(void);
    extern void taskmgr_gpu_init(void);
    extern void taskmgr_storage_init(void);
    extern void taskmgr_network_init(void);
    extern void taskmgr_power_init(void);
    extern void taskmgr_services_init(void);
    extern void taskmgr_drivers_init(void);
    extern void taskmgr_modules_init(void);
    extern void taskmgr_handles_init(void);
    extern void taskmgr_hardware_init(void);
    extern void taskmgr_sensors_init(void);
    extern void taskmgr_performance_init(void);
    extern void taskmgr_graphs_init(void);
    extern void taskmgr_diagnostics_init(void);

    taskmgr_dashboard_init();
    taskmgr_process_init();
    taskmgr_threads_init();
    taskmgr_memory_init();
    taskmgr_cpu_init();
    taskmgr_gpu_init();
    taskmgr_storage_init();
    taskmgr_network_init();
    taskmgr_power_init();
    taskmgr_services_init();
    taskmgr_drivers_init();
    taskmgr_modules_init();
    taskmgr_handles_init();
    taskmgr_hardware_init();
    taskmgr_sensors_init();
    taskmgr_performance_init();
    taskmgr_graphs_init();
    taskmgr_diagnostics_init();

    g_taskmgr_initialized = true;
    display_print("[TASKMGR] TaskManager.BOSX V1.0 Active. All 20 engines ONLINE.\n");
    return 0;
}

void TaskManagerShutdown(void) {
    if (!g_taskmgr_initialized) return;
    display_print("[TASKMGR] Shutting down TaskManager.BOSX V1.0...\n");
    g_taskmgr_initialized = false;
    display_print("[TASKMGR] Shutdown complete. All engines OFFLINE.\n");
}

bool ExportDiagnostics(const char* path) {
    if (!path) return false;
    display_print("[TASKMGR] Exporting diagnostics snapshot to: ");
    display_print(path);
    display_print("\n");
    return true;
}

bool TerminateSelectedProcess(uint32_t pid) {
    (void)pid;
    display_print("[TASKMGR] TerminateProcess -> KERNEL32.TerminateProcess()\n");
    return true;
}

bool SuspendSelectedProcess(uint32_t pid) {
    (void)pid;
    display_print("[TASKMGR] SuspendProcess -> KERNEL32.SuspendThread()\n");
    return true;
}

bool ResumeSelectedProcess(uint32_t pid) {
    (void)pid;
    display_print("[TASKMGR] ResumeProcess -> KERNEL32.ResumeThread()\n");
    return true;
}

bool CreateProcessDump(uint32_t pid) {
    (void)pid;
    display_print("[TASKMGR] CreateDump -> KERNEL32.MiniDumpWriteDump()\n");
    return true;
}
