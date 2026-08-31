/*
 * ATOMS OS — Phase 12 Chromium Net + Storage Verification Test Suite
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * 34 deterministic tests covering:
 *   - GURL parsing & canonicalization
 *   - SecurityOrigin creation & isolation
 *   - HTTP request/response headers
 *   - CanonicalCookie RFC 6265 parsing & matching
 *   - CookieStore operations
 *   - HttpCache store/lookup/invalidation
 *   - URLLoader pipeline integrity
 *   - StorageArea key-value operations & quota
 *   - StorageNamespace origin isolation
 *   - LocalStorageManager & SessionStorageManager
 *   - VFS adapter serialization format & CRC32
 *   - Cross-origin isolation enforcement
 */

#ifndef THIRD_PARTY_CHROMIUM_NET_TESTS_NET_STORAGE_TEST_SUITE_H_
#define THIRD_PARTY_CHROMIUM_NET_TESTS_NET_STORAGE_TEST_SUITE_H_

#include <stdint.h>

struct NetStorageTestResult {
    const char* test_name;
    bool passed;
    const char* detail;
};

#ifdef __cplusplus
extern "C" {
#endif

bool NetStorage_RunAllVerificationTests(void);

#ifdef __cplusplus
}
#endif

int RunNetStorageTestSuite(NetStorageTestResult* results, int max_results);

#endif // THIRD_PARTY_CHROMIUM_NET_TESTS_NET_STORAGE_TEST_SUITE_H_
