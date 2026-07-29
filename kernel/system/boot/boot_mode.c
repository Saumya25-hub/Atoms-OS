/**
 * @file boot_mode.c
 * @brief BOS OS Unified Boot Mode & Diagnostic Gating Implementation
 */

#include "kernel/system/boot/boot_mode.h"
#include <stddef.h>

BOS_BootMode g_bos_boot_mode = BOS_BOOT_MODE_RELEASE;
bool         g_bos_enable_profiler_logs = false;

extern void display_print(const char* s);

static bool str_contains(const char* str, const char* sub) {
    if (!str || !sub) return false;
    for (int i = 0; str[i] != '\0'; i++) {
        bool match = true;
        for (int j = 0; sub[j] != '\0'; j++) {
            if (str[i + j] != sub[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

void BOS_BootMode_Init(const char* cmdline) {
    g_bos_boot_mode = BOS_BOOT_MODE_RELEASE;
    g_bos_enable_profiler_logs = false;

    if (cmdline) {
        if (str_contains(cmdline, "bos.test=1")) {
            g_bos_boot_mode = BOS_BOOT_MODE_TEST;
            display_print("[BOOT_MODE] TEST MODE ACTIVATED (Synchronous Certification Suite)\n");
        } else if (str_contains(cmdline, "bos.debug=1")) {
            g_bos_boot_mode = BOS_BOOT_MODE_DEBUG;
            display_print("[BOOT_MODE] DEBUG MODE ACTIVATED (Verbose Autopsy Logs Enabled)\n");
        } else {
            display_print("[BOOT_MODE] RELEASE MODE ACTIVE (Instant Boot & Deferred Idle Tasks)\n");
        }

        if (str_contains(cmdline, "bos.profile=1")) {
            g_bos_enable_profiler_logs = true;
        }
    } else {
        display_print("[BOOT_MODE] RELEASE MODE ACTIVE (Instant Boot & Deferred Idle Tasks)\n");
    }
}

void BOS_BootMode_Set(BOS_BootMode mode) {
    g_bos_boot_mode = mode;
}

bool BOS_IsReleaseMode(void) {
    return g_bos_boot_mode == BOS_BOOT_MODE_RELEASE;
}

bool BOS_IsDebugMode(void) {
    return g_bos_boot_mode == BOS_BOOT_MODE_DEBUG;
}

bool BOS_IsTestMode(void) {
    return g_bos_boot_mode == BOS_BOOT_MODE_TEST;
}
