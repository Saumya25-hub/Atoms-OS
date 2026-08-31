/*
 * ATOMS OS — Google V8 Test Runner Master Executable
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "third_party/v8/tests/v8_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[V8_TEST_RUNNER] Starting Phase 10 Google V8 Verification Suite...");
    bool ok = V8_RunAllVerificationTests();
    return ok ? 0 : 1;
}
