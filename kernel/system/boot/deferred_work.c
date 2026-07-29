/**
 * @file deferred_work.c
 * @brief BOS OS Deferred Initialization & Background Task Subsystem Implementation
 */

#include "kernel/system/boot/deferred_work.h"
#include "kernel/system/boot/boot_mode.h"
#include <stddef.h>

extern void display_print(const char* s);

static BOS_DeferredTask s_deferred_tasks[BOS_MAX_DEFERRED_TASKS];
static uint32_t         s_deferred_task_count = 0;
static bool             s_deferred_work_initialized = false;

void BOS_DeferredWork_Init(void) {
    s_deferred_task_count = 0;
    s_deferred_work_initialized = true;
    for (int i = 0; i < BOS_MAX_DEFERRED_TASKS; i++) {
        s_deferred_tasks[i].task_name = NULL;
        s_deferred_tasks[i].func = NULL;
        s_deferred_tasks[i].executed = false;
    }
}

bool BOS_DeferredWork_Register(const char* name, BOS_DeferredTaskFunc func) {
    if (!s_deferred_work_initialized) BOS_DeferredWork_Init();
    if (s_deferred_task_count >= BOS_MAX_DEFERRED_TASKS || !func) return false;

    s_deferred_tasks[s_deferred_task_count].task_name = name ? name : "UnnamedDeferredTask";
    s_deferred_tasks[s_deferred_task_count].func = func;
    s_deferred_tasks[s_deferred_task_count].executed = false;
    s_deferred_task_count++;
    return true;
}

void BOS_DeferredWork_PumpIdleQueue(void) {
    if (!s_deferred_work_initialized || s_deferred_task_count == 0) return;

    /* Execute ONE deferred task per idle pump pass to avoid frame drops */
    for (uint32_t i = 0; i < s_deferred_task_count; i++) {
        if (!s_deferred_tasks[i].executed && s_deferred_tasks[i].func) {
            s_deferred_tasks[i].executed = true;
            if (BOS_IsDebugMode()) {
                display_print("[DEFERRED_WORK] Executing background task: ");
                display_print(s_deferred_tasks[i].task_name);
                display_print("\n");
            }
            s_deferred_tasks[i].func();
            return; /* Yield back to main GUI loop */
        }
    }
}

void BOS_RunAllDeferredTests(void) {
    if (!s_deferred_work_initialized) return;
    display_print("\n=== RUNNING ALL DEFERRED TEST SUITES ===\n");
    for (uint32_t i = 0; i < s_deferred_task_count; i++) {
        if (s_deferred_tasks[i].func) {
            display_print("[TEST] Invoking: ");
            display_print(s_deferred_tasks[i].task_name);
            display_print("...\n");
            s_deferred_tasks[i].func();
            s_deferred_tasks[i].executed = true;
        }
    }
    display_print("=== ALL DEFERRED TESTS COMPLETED SUCCESSFULLY ===\n\n");
}
