/*
 * ATOMS OS — Phase 17 Web Compatibility & Hardening Standalone Test Runner
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "compatibility_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    puts("\n[ATRIX BROWSER] Starting Phase 17 Web Compatibility & Hardening Verification Suite...");
    bool success = Compatibility_RunAllVerificationTests();

    if (success) {
        puts("[ATRIX BROWSER] Phase 17 Compatibility & Hardening PASSED with 100% success.\n");
        return 0;
    } else {
        puts("[ATRIX BROWSER] Phase 17 Compatibility & Hardening FAILED.\n");
        return 1;
    }
}
