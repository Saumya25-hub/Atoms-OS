/*
 * ATOMS OS — Phase 13 Multi-Process Browser Verification Test Suite Header
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef THIRD_PARTY_CHROMIUM_PROCESS_TESTS_PROCESS_TEST_SUITE_H_
#define THIRD_PARTY_CHROMIUM_PROCESS_TESTS_PROCESS_TEST_SUITE_H_

#include <stdint.h>
#include <stdbool.h>

struct ProcessTestResult {
    const char* test_name;
    bool passed;
    const char* detail;
};

#ifdef __cplusplus
extern "C" {
#endif

bool Process_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

int RunProcessTestSuite(ProcessTestResult* results, int max_results);

#endif // THIRD_PARTY_CHROMIUM_PROCESS_TESTS_PROCESS_TEST_SUITE_H_
