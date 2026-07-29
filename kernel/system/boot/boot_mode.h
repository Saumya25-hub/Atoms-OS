/**
 * @file boot_mode.h
 * @brief BOS OS Unified Boot Mode & Diagnostic Gating Subsystem
 * @status Production Kernel Architecture Component
 */

#ifndef BOS_BOOT_MODE_H
#define BOS_BOOT_MODE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOS_BOOT_MODE_RELEASE = 0, /* Default: Fast instant boot, zero serial spam, deferred idle tests */
    BOS_BOOT_MODE_DEBUG   = 1, /* Verbose logs, assertions enabled, window autopsy ON */
    BOS_BOOT_MODE_TEST    = 2  /* Run ALL certification & stress tests synchronously during boot */
} BOS_BootMode;

extern BOS_BootMode g_bos_boot_mode;
extern bool         g_bos_enable_profiler_logs;

void BOS_BootMode_Init(const char* cmdline);
void BOS_BootMode_Set(BOS_BootMode mode);

bool BOS_IsReleaseMode(void);
bool BOS_IsDebugMode(void);
bool BOS_IsTestMode(void);

#endif /* BOS_BOOT_MODE_H */
