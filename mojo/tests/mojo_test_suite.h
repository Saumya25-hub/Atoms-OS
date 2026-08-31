/*
 * ATOMS OS — Phase 14 Mojo IPC Verification Test Suite Header
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef MOJO_TESTS_MOJO_TEST_SUITE_H_
#define MOJO_TESTS_MOJO_TEST_SUITE_H_

#include <stdint.h>
#include <stdbool.h>

struct MojoTestResult {
    const char* test_id;
    const char* test_name;
    bool passed;
    const char* detail;
};

#ifdef __cplusplus
extern "C" {
#endif

bool Mojo_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

int RunMojoTestSuite(MojoTestResult* results, int max_results);

#endif // MOJO_TESTS_MOJO_TEST_SUITE_H_
