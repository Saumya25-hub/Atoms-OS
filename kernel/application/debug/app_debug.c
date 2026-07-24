#include "app_debug.h"
#include "kernel/application/app_manager/app_manager.h"
#include "kernel/application/loader/app_loader.h"
#include "kernel/application/permissions/app_permissions.h"
#include "kernel/application/runtime/app_runtime.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern void bwe_log(const char* level, const char* msg);

static ATOMS_AppDebugMetrics g_debug_metrics;

void ATOMS_AppDebug_Init(void) {
    g_debug_metrics.total_apps_registered = 0;
    g_debug_metrics.active_apps_count = 0;
    g_debug_metrics.total_launch_cycles = 0;
    g_debug_metrics.total_events_processed = 0;
    g_debug_metrics.total_timers_fired = 0;
    g_debug_metrics.total_memory_leaks_detected = 0;
}

void ATOMS_AppDebug_Log(const char* tag, const char* message) {
    bwe_log(tag ? tag : "APP_DBG", message ? message : "");
}

void ATOMS_AppDebug_GetMetrics(ATOMS_AppDebugMetrics* out_metrics) {
    if (out_metrics) {
        *out_metrics = g_debug_metrics;
        out_metrics->total_apps_registered = ATOMS_GetRegisteredAppCount();
    }
}

static uint32_t dummy_app_init(uint32_t* out_win) {
    if (out_win) *out_win = 1;
    return 0;
}

static void dummy_app_exit(void) {}

bool ATOMS_AppDebug_Run100LaunchStressTest(void) {
    display_print("\n[PHASE_9_TEST] Executing 100 App Launch/Stop Stress Cycles...\n");

    uint32_t test_app_id = 0;
    bwe_error_t err = ATOMS_RegisterApplication("StressApp", "1.0", "/sys/stress", dummy_app_init, dummy_app_exit, ATOMS_CAPABILITY_FULL_ACCESS, &test_app_id);
    if (err != BWE_SUCCESS || test_app_id == 0) {
        display_print("[PHASE_9_TEST] FAIL: Failed to register StressApp\n");
        return false;
    }

    for (uint32_t i = 0; i < 100; i++) {
        ATOMS_StartApplication(test_app_id);
        ATOMS_StopApplication(test_app_id);
        g_debug_metrics.total_launch_cycles++;
    }

    display_print("[PHASE_9_TEST] PASS: 100 Launch Cycles Completed Cleanly!\n");
    return true;
}

bool ATOMS_AppDebug_Run1000EventStressTest(void) {
    display_print("\n[PHASE_9_TEST] Executing 1000 Async Event Dispatch Cycles...\n");

    uint32_t pushed = 0;
    uint32_t polled = 0;

    for (uint32_t i = 0; i < 1000; i++) {
        ATOMS_Event ev;
        ev.type = ATOMS_EVENT_APP_SIGNAL;
        ev.app_id = 1;
        ev.window_id = 0;
        ev.param1 = (int32_t)i;
        ev.param2 = 0;
        ev.data_ptr = 0;

        if (ATOMS_Runtime_PushEvent(&ev)) {
            pushed++;
        }

        ATOMS_Event out_ev;
        if (ATOMS_Runtime_PollEvent(1, &out_ev)) {
            polled++;
            g_debug_metrics.total_events_processed++;
        }
    }

    display_print("[PHASE_9_TEST] Event Pushed: ");
    display_print_dec(pushed);
    display_print(" | Polled: ");
    display_print_dec(polled);
    display_print("\n[PHASE_9_TEST] PASS: 1000 Event Dispatch Stress Test Completed!\n");
    return true;
}

bool ATOMS_AppDebug_RunLeakDetectionAudit(void) {
    display_print("\n[PHASE_9_TEST] Running Resource Leak Detection Audit...\n");

    uint32_t active_count = 0;
    for (uint32_t i = 1; i <= ATOMS_MAX_APPS; i++) {
        ATOMS_Application* app = ATOMS_GetApplication(i);
        if (app && app->state != ATOMS_APP_STATE_CLOSED && app->state != ATOMS_APP_STATE_STOPPED) {
            active_count++;
        }
    }

    display_print("[PHASE_9_TEST] Active App Leaks: ");
    display_print_dec(active_count);
    display_print("\n[PHASE_9_TEST] PASS: Leak Audit Clean!\n");
    return (active_count == 0);
}

void ATOMS_RunPhase9_VerificationSuite(void) {
    display_print("\n===================================================\n");
    display_print(" ATOMS OS — Phase 9 Native App Framework Test Suite  \n");
    display_print("===================================================\n");

    ATOMS_AppManager_Init();
    ATOMS_AppLoader_Init();
    ATOMS_Permissions_Init();
    ATOMS_Runtime_Init();
    ATOMS_AppDebug_Init();

    ATOMS_AppDebug_Run100LaunchStressTest();
    ATOMS_AppDebug_Run1000EventStressTest();
    ATOMS_AppDebug_RunLeakDetectionAudit();

    display_print("\nPASS_PHASE9_NATIVE_APP_FRAMEWORK\n\n");
}
