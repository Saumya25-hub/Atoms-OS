/*
 * ATOMS OS / ATRIX — Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Forensic Isolation Test Suite (Phases A–J)
 */

#ifndef THIRD_PARTY_ATRIX_MINIMAL_BROWSER_ISOLATION_TESTS_H_
#define THIRD_PARTY_ATRIX_MINIMAL_BROWSER_ISOLATION_TESTS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* test_name;
    const char* target_url;
    bool passed;
    int http_status;
    uint32_t body_bytes;
    uint32_t stage_reached;
    const char* failure_category;
    const char* detail;
} BrowserIsolationResult;

// Run all 6 Isolation Stages for a given URL
bool RunIsolationStageTests(const char* target_url, BrowserIsolationResult* out_results, int max_results, int* out_count);

// Run full test battery (Google.com + Example.com control)
bool RunFullMinimalBrowserBattery(void);

// Forensic Crash Log Capture
void MinimalBrowser_CaptureCrashContext(uint32_t vector, uint64_t error_code, uint64_t rip, uint64_t rsp, uint64_t cr2, uint64_t cr3);

#ifdef __cplusplus
}
#endif

#endif // THIRD_PARTY_ATRIX_MINIMAL_BROWSER_ISOLATION_TESTS_H_
