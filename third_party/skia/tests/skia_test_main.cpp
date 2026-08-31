/*
 * ATOMS OS — Skia 2D Test Runner Master Executable
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "third_party/skia/tests/skia_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[SKIA_TEST_RUNNER] Starting Phase 9 Skia Verification Suite...");
    bool ok = Skia_RunAllVerificationTests();
    return ok ? 0 : 1;
}
