/*
 * ATOMS OS — Phase 16 Media, GPU & Advanced Web APIs Verification Suite Header
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef THIRD_PARTY_CHROMIUM_MEDIA_GPU_TESTS_MEDIA_GPU_TEST_SUITE_H_
#define THIRD_PARTY_CHROMIUM_MEDIA_GPU_TESTS_MEDIA_GPU_TEST_SUITE_H_

#include <stdint.h>
#include <stdbool.h>

struct MediaGpuTestResult {
    const char* test_id;
    const char* test_name;
    bool passed;
    const char* detail;
};

#ifdef __cplusplus
extern "C" {
#endif

bool MediaGpu_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

int RunMediaGpuTestSuite(MediaGpuTestResult* results, int max_results);

#endif // THIRD_PARTY_CHROMIUM_MEDIA_GPU_TESTS_MEDIA_GPU_TEST_SUITE_H_
