/*
 * ATOMS OS — Phase 12 Net + Storage Test Runner Master Executable
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "net_storage_test_suite.h"
#include "userspace/runtime/c/include/stdio.h"

extern "C" int main(void) {
    puts("[NET_STORAGE_TEST_RUNNER] Starting Phase 12 Chromium Networking & Storage Verification Suite...");
    bool ok = NetStorage_RunAllVerificationTests();
    return ok ? 0 : 1;
}
