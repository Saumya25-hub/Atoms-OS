/*
 * ATOMS OS — Phase 15 Sandbox + Web Security Verification Test Suite Header
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef CHROMIUM_SECURITY_TESTS_SECURITY_TEST_SUITE_H_
#define CHROMIUM_SECURITY_TESTS_SECURITY_TEST_SUITE_H_

#include <stdint.h>
#include <stdbool.h>

struct SecurityTestResult {
    const char* test_id;
    const char* test_name;
    bool passed;
    const char* detail;
};

#ifdef __cplusplus
extern "C" {
#endif

bool Security_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

int RunSecurityTestSuite(SecurityTestResult* results, int max_results);

#endif // CHROMIUM_SECURITY_TESTS_SECURITY_TEST_SUITE_H_
