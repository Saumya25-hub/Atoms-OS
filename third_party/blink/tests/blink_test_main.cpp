/*
 * ATOMS OS — Google Chromium / Blink Test Runner Master Executable
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "third_party/blink/tests/blink_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[BLINK_TEST_RUNNER] Starting Phase 11 Chromium Blink Core Verification Suite...");
    bool ok = Blink_RunAllVerificationTests();
    return ok ? 0 : 1;
}
