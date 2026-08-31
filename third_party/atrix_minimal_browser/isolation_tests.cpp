/*
 * ATOMS OS / ATRIX — Minimal Real-Web Browser Probe
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implementation of Forensic Isolation Tests (Phases A–J)
 */

#include "isolation_tests.h"
#include "minimal_browser.h"
#include <stdio.h>
#include <string.h>

extern "C" {
extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);
extern void com1_puts(const char* s);
}

static void RecordIsolationResult(BrowserIsolationResult* r, int idx, const char* name, const char* url,
                                  bool passed, int status, uint32_t bytes, uint32_t stage,
                                  const char* category, const char* detail) {
    if (!r) return;
    r[idx].test_name = name;
    r[idx].target_url = url;
    r[idx].passed = passed;
    r[idx].http_status = status;
    r[idx].body_bytes = bytes;
    r[idx].stage_reached = stage;
    r[idx].failure_category = category;
    r[idx].detail = detail;
}

bool RunIsolationStageTests(const char* target_url, BrowserIsolationResult* out_results, int max_results, int* out_count) {
    if (!target_url || !out_results || max_results < 6) return false;

    printf("\n=======================================================\n");
    printf("   RUNNING ISOLATION STAGE BATTERY FOR: %s\n", target_url);
    printf("=======================================================\n");

    atrix::MinimalBrowser browser(1024, 640);
    browser.Initialize();

    int test_idx = 0;

    // ------------------------------------------------------------
    // TEST 1: Real HTTPS request, response received, rendering disabled
    // ------------------------------------------------------------
    printf("\n[TEST 1] Initiating Real HTTPS Request (No Rendering)...\n");
    bool t1_pass = browser.ExecuteStage1_NetworkOnly(target_url);
    const atrix::MinimalBrowserStatus& st1 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 1: Real HTTPS Network Only", target_url,
                          t1_pass, st1.http_status_code, (uint32_t)st1.response_body_bytes, 1,
                          t1_pass ? "NONE" : (st1.failure_reason == atrix::FAIL_DNS ? "DNS_FAILURE" : "TCP/TLS_FAILURE"),
                          t1_pass ? "HTTPS Response received successfully" : "Network connection or handshake failed");

    // ------------------------------------------------------------
    // TEST 2: Real HTTPS request + Blink HTML Parser & DOM only
    // ------------------------------------------------------------
    printf("\n[TEST 2] Initiating Real HTTPS + HTML Parser / DOM Tree...\n");
    bool t2_pass = browser.ExecuteStage2_HTMLParseOnly(target_url);
    const atrix::MinimalBrowserStatus& st2 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 2: Real HTTPS + HTML Parser / DOM", target_url,
                          t2_pass, st2.http_status_code, (uint32_t)st2.response_body_bytes, 2,
                          t2_pass ? "NONE" : "HTML_PARSER_FAILURE",
                          t2_pass ? "HTML tokenized and DOM constructed cleanly" : "DOM tree construction failed");

    // ------------------------------------------------------------
    // TEST 3: Real HTTPS request + DOM + CSS / Layout Tree
    // ------------------------------------------------------------
    printf("\n[TEST 3] Initiating Real HTTPS + DOM + CSS / Layout...\n");
    bool t3_pass = browser.ExecuteStage3_DOMAndLayoutOnly(target_url);
    const atrix::MinimalBrowserStatus& st3 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 3: Real HTTPS + DOM + CSS / Layout", target_url,
                          t3_pass, st3.http_status_code, (uint32_t)st3.response_body_bytes, 3,
                          t3_pass ? "NONE" : "LAYOUT_FAILURE",
                          t3_pass ? "Layout tree built and box model computed" : "Layout computation failed");

    // ------------------------------------------------------------
    // TEST 4: Real HTTPS request + Skia rendering (No BWE)
    // ------------------------------------------------------------
    printf("\n[TEST 4] Initiating Real HTTPS + Skia In-Memory Rasterization...\n");
    bool t4_pass = browser.ExecuteStage4_SkiaInMemory(target_url);
    const atrix::MinimalBrowserStatus& st4 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 4: Real HTTPS + Skia (No BWE)", target_url,
                          t4_pass, st4.http_status_code, (uint32_t)st4.response_body_bytes, 4,
                          t4_pass ? "NONE" : "SKIA_PAINT_FAILURE",
                          t4_pass ? "Skia software rasterization completed cleanly" : "Skia paint failed");

    // ------------------------------------------------------------
    // TEST 5: Real HTTPS request + Skia + BWE Surface
    // ------------------------------------------------------------
    printf("\n[TEST 5] Initiating Real HTTPS + Skia + BWE Surface...\n");
    bool t5_pass = browser.ExecuteStage5_SkiaBWE(target_url);
    const atrix::MinimalBrowserStatus& st5 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 5: Real HTTPS + Skia + BWE Surface", target_url,
                          t5_pass, st5.http_status_code, (uint32_t)st5.response_body_bytes, 5,
                          t5_pass ? "NONE" : "BWE_SURFACE_FAILURE",
                          t5_pass ? "BWE surface invalidated and rendered" : "BWE surface pipeline error");

    // ------------------------------------------------------------
    // TEST 6: Full Minimal Browser Window + BWE + Input
    // ------------------------------------------------------------
    printf("\n[TEST 6] Testing Full Minimal Browser Interactive Pipeline...\n");
    bool t6_pass = browser.Navigate(target_url);
    const atrix::MinimalBrowserStatus& st6 = browser.GetStatus();
    RecordIsolationResult(out_results, test_idx++, "TEST 6: Full Minimal Browser Interactive", target_url,
                          t6_pass, st6.http_status_code, (uint32_t)st6.response_body_bytes, 6,
                          t6_pass ? "NONE" : "BROWSER_PIPELINE_FAILURE",
                          t6_pass ? "Full pipeline navigated and interactive" : "Interactive navigation error");

    if (out_count) *out_count = test_idx;
    return true;
}

bool RunFullMinimalBrowserBattery(void) {
    puts("\n=======================================================");
    puts("   ATRIX MINIMAL REAL-WEB BROWSER: ISOLATION BATTERY   ");
    puts("=======================================================");

    BrowserIsolationResult results[16];
    int google_count = 0;
    int example_count = 0;

    // 1. Primary Target: Google.com
    RunIsolationStageTests("https://www.google.com/", results, 16, &google_count);

    // 2. Control Target: Example.com
    RunIsolationStageTests("https://example.com/", &results[google_count], 16 - google_count, &example_count);

    int total = google_count + example_count;
    int passed = 0;

    puts("\n=======================================================");
    puts("               ISOLATION RESULTS SUMMARY               ");
    puts("=======================================================");

    for (int i = 0; i < total; i++) {
        printf("[%s] on %s ... ", results[i].test_name, results[i].target_url);
        if (results[i].passed) {
            printf("PASS (HTTP %d, %u bytes, Detail: %s)\n",
                   results[i].http_status, results[i].body_bytes, results[i].detail);
            passed++;
        } else {
            printf("FAIL (Category: %s, Detail: %s)\n",
                   results[i].failure_category, results[i].detail);
        }
    }

    printf("\nFINAL BATTERY SCORE: %d/%d PASSED\n", passed, total);
    puts("=======================================================\n");

    return (passed == total);
}

void MinimalBrowser_CaptureCrashContext(uint32_t vector, uint64_t error_code, uint64_t rip, uint64_t rsp, uint64_t cr2, uint64_t cr3) {
    printf("\n[MINIMAL_BROWSER][CRASH_CAPTURE] ========================\n");
    printf("Vector     : #%u\n", vector);
    printf("Error Code : 0x%llx\n", (unsigned long long)error_code);
    printf("RIP        : 0x%016llx\n", (unsigned long long)rip);
    printf("RSP        : 0x%016llx\n", (unsigned long long)rsp);
    printf("CR2        : 0x%016llx\n", (unsigned long long)cr2);
    printf("CR3        : 0x%016llx\n", (unsigned long long)cr3);
    printf("Subsystem  : ATRIX Minimal Real-Web Browser Probe\n");
    printf("========================================================\n\n");
}
