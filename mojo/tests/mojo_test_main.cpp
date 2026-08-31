/*
 * ATOMS OS — Phase 14 Mojo Test Runner Master Executable
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "mojo_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[MOJO_TEST_RUNNER] Starting Phase 14 Mojo IPC Verification Suite...");
    bool ok = Mojo_RunAllVerificationTests();
    return ok ? 0 : 1;
}
