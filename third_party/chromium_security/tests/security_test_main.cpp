/*
 * ATOMS OS — Phase 15 Security Test Runner Master Executable
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "security_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[SECURITY_TEST_RUNNER] Starting Phase 15 Sandbox & Web Security Verification Suite...");
    bool ok = Security_RunAllVerificationTests();
    return ok ? 0 : 1;
}
