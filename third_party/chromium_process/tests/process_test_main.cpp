/*
 * ATOMS OS — Phase 13 Multi-Process Browser Test Runner Master Executable
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "process_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[PROCESS_TEST_RUNNER] Starting Phase 13 Multi-Process Browser Verification Suite...");
    bool ok = Process_RunAllVerificationTests();
    return ok ? 0 : 1;
}
