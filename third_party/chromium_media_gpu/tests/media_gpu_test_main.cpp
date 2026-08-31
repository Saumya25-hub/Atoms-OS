/*
 * ATOMS OS — Phase 16 Media & GPU Test Runner Master Executable
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "media_gpu_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[MEDIA_GPU_TEST_RUNNER] Starting Phase 16 Media, GPU & Advanced Web APIs Verification Suite...");
    bool ok = MediaGpu_RunAllVerificationTests();
    return ok ? 0 : 1;
}
