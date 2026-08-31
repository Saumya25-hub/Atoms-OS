/*
 * ATOMS OS — Phase 17 Web Compatibility & Hardening Verification Suite
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Target: 46 Deterministic Tests (T01–T46)
 */

#ifndef THIRD_PARTY_CHROMIUM_COMPATIBILITY_TESTS_COMPATIBILITY_TEST_SUITE_H_
#define THIRD_PARTY_CHROMIUM_COMPATIBILITY_TESTS_COMPATIBILITY_TEST_SUITE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* test_id;
    const char* test_name;
    bool passed;
    const char* detail;
} CompatibilityTestResult;

int RunCompatibilityTestSuite(CompatibilityTestResult* results, int max_results);
bool Compatibility_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

#endif // THIRD_PARTY_CHROMIUM_COMPATIBILITY_TESTS_COMPATIBILITY_TEST_SUITE_H_
