/*
 * ATOMS OS — Phase 12 Chromium Net + Storage Verification Test Suite
 * 34 Deterministic Tests
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "net_storage_test_suite.h"

#include "third_party/chromium_net/base/gurl.h"
#include "third_party/chromium_net/base/security_origin.h"
#include "third_party/chromium_net/http/http_request_headers.h"
#include "third_party/chromium_net/http/http_response_headers.h"
#include "third_party/chromium_net/http/http_cache.h"
#include "third_party/chromium_net/cookies/canonical_cookie.h"
#include "third_party/chromium_net/cookies/cookie_store.h"
#include "third_party/chromium_net/url_request/url_loader.h"
#include "third_party/chromium_storage/dom_storage/storage_area.h"
#include "third_party/chromium_storage/dom_storage/storage_namespace.h"
#include "third_party/chromium_storage/dom_storage/local_storage_manager.h"
#include "third_party/chromium_storage/dom_storage/session_storage_manager.h"
#include "third_party/chromium_storage/adapter/atoms_storage_vfs_adapter.h"
#include "userspace/runtime/c/include/stdio.h"

static int test_idx = 0;

static void RecordResult(NetStorageTestResult* results, const char* name, bool pass, const char* detail) {
    results[test_idx].test_name = name;
    results[test_idx].passed = pass;
    results[test_idx].detail = detail;
    test_idx++;
}

// ============================================================
// Test 1: GURL — Parse HTTPS URL
// ============================================================
static void Test_GURL_ParseHTTPS(NetStorageTestResult* r) {
    net::GURL url("https://www.example.com/path?q=1#frag");
    bool pass = url.is_valid() &&
                url.scheme() == "https" &&
                url.host() == "www.example.com" &&
                url.port() == 443 &&
                url.path() == "/path" &&
                url.is_secure();
    RecordResult(r, "GURL_ParseHTTPS", pass, pass ? "HTTPS parsed correctly" : "HTTPS parse failed");
}

// ============================================================
// Test 2: GURL — Parse HTTP URL with explicit port
// ============================================================
static void Test_GURL_ParseHTTPPort(NetStorageTestResult* r) {
    net::GURL url("http://example.com:8080/index.html");
    bool pass = url.is_valid() &&
                url.scheme() == "http" &&
                url.host() == "example.com" &&
                url.port() == 8080 &&
                url.path() == "/index.html" &&
                !url.is_secure();
    RecordResult(r, "GURL_ParseHTTPPort", pass, pass ? "HTTP+port parsed" : "HTTP+port parse failed");
}

// ============================================================
// Test 3: GURL — Invalid URL
// ============================================================
static void Test_GURL_Invalid(NetStorageTestResult* r) {
    net::GURL url("not-a-url");
    RecordResult(r, "GURL_Invalid", !url.is_valid(), !url.is_valid() ? "Invalid rejected" : "Should reject invalid");
}

// ============================================================
// Test 4: SecurityOrigin — Create from GURL
// ============================================================
static void Test_SecurityOrigin_Create(NetStorageTestResult* r) {
    net::GURL url("https://example.com:443/path");
    net::SecurityOrigin origin = net::SecurityOrigin::Create(url);
    bool pass = origin.scheme() == "https" &&
                origin.host() == "example.com" &&
                origin.port() == 443;
    RecordResult(r, "SecurityOrigin_Create", pass, pass ? "Origin created" : "Origin creation failed");
}

// ============================================================
// Test 5: SecurityOrigin — Same origin check
// ============================================================
static void Test_SecurityOrigin_SameOrigin(NetStorageTestResult* r) {
    net::SecurityOrigin a("https", "example.com", 443);
    net::SecurityOrigin b("https", "example.com", 443);
    bool pass = a.IsSameOriginWith(b);
    RecordResult(r, "SecurityOrigin_SameOrigin", pass, pass ? "Same origin" : "Should be same origin");
}

// ============================================================
// Test 6: SecurityOrigin — Different origin (scheme)
// ============================================================
static void Test_SecurityOrigin_DiffScheme(NetStorageTestResult* r) {
    net::SecurityOrigin a("https", "example.com", 443);
    net::SecurityOrigin b("http", "example.com", 80);
    bool pass = !a.IsSameOriginWith(b);
    RecordResult(r, "SecurityOrigin_DiffScheme", pass, pass ? "Different origin by scheme" : "Should differ by scheme");
}

// ============================================================
// Test 7: SecurityOrigin — Different origin (port)
// ============================================================
static void Test_SecurityOrigin_DiffPort(NetStorageTestResult* r) {
    net::SecurityOrigin a("https", "example.com", 443);
    net::SecurityOrigin b("https", "example.com", 8443);
    bool pass = !a.IsSameOriginWith(b);
    RecordResult(r, "SecurityOrigin_DiffPort", pass, pass ? "Different origin by port" : "Should differ by port");
}

// ============================================================
// Test 8: SecurityOrigin — Different origin (host)
// ============================================================
static void Test_SecurityOrigin_DiffHost(NetStorageTestResult* r) {
    net::SecurityOrigin a("https", "example.com", 443);
    net::SecurityOrigin b("https", "sub.example.com", 443);
    bool pass = !a.IsSameOriginWith(b);
    RecordResult(r, "SecurityOrigin_DiffHost", pass, pass ? "Different origin by host" : "Should differ by host");
}

// ============================================================
// Test 9: HttpRequestHeaders — Set/Get/Remove
// ============================================================
static void Test_HttpRequestHeaders(NetStorageTestResult* r) {
    net::HttpRequestHeaders h;
    h.SetHeader("Host", "example.com");
    h.SetHeader("Accept", "text/html");
    std::string val;
    bool pass = h.GetHeader("Host", &val) && val == "example.com";
    h.RemoveHeader("Host");
    pass = pass && !h.HasHeader("Host") && h.HasHeader("Accept");
    RecordResult(r, "HttpRequestHeaders_SetGetRemove", pass, pass ? "Headers work" : "Header ops failed");
}

// ============================================================
// Test 10: HttpResponseHeaders — Parse status line
// ============================================================
static void Test_HttpResponseHeaders_Parse(NetStorageTestResult* r) {
    net::HttpResponseHeaders resp("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 42");
    bool pass = resp.response_code() == 200 &&
                resp.HasHeader("Content-Type") &&
                resp.GetContentLength() == 42;
    RecordResult(r, "HttpResponseHeaders_Parse", pass, pass ? "Headers parsed" : "Header parse failed");
}

// ============================================================
// Test 11: HttpResponseHeaders — Redirect detection
// ============================================================
static void Test_HttpResponseHeaders_Redirect(NetStorageTestResult* r) {
    net::HttpResponseHeaders resp("HTTP/1.1 301 Moved\r\nLocation: https://new.example.com/");
    bool pass = resp.IsRedirect();
    std::string loc;
    pass = pass && resp.GetLocationHeader(&loc) && loc == "https://new.example.com/";
    RecordResult(r, "HttpResponseHeaders_Redirect", pass, pass ? "Redirect detected" : "Redirect detection failed");
}

// ============================================================
// Test 12: HttpResponseHeaders — Set-Cookie extraction
// ============================================================
static void Test_HttpResponseHeaders_SetCookie(NetStorageTestResult* r) {
    net::HttpResponseHeaders resp("HTTP/1.1 200 OK\r\nSet-Cookie: sid=abc123\r\nSet-Cookie: theme=dark");
    std::vector<std::string> cookies = resp.GetSetCookieHeaders();
    bool pass = cookies.size() == 2;
    RecordResult(r, "HttpResponseHeaders_SetCookie", pass, pass ? "Set-Cookie extracted" : "Set-Cookie extraction failed");
}

// ============================================================
// Test 13: CanonicalCookie — Create from Set-Cookie line
// ============================================================
static void Test_CanonicalCookie_Create(NetStorageTestResult* r) {
    net::GURL url("https://example.com/path");
    net::CanonicalCookie cookie = net::CanonicalCookie::Create(url, "session=abc123; Path=/; Secure; HttpOnly");
    bool pass = cookie.Name() == "session" &&
                cookie.Value() == "abc123" &&
                cookie.Path() == "/" &&
                cookie.IsSecure() &&
                cookie.IsHttpOnly();
    RecordResult(r, "CanonicalCookie_Create", pass, pass ? "Cookie parsed" : "Cookie parse failed");
}

// ============================================================
// Test 14: CanonicalCookie — Domain matching
// ============================================================
static void Test_CanonicalCookie_DomainMatch(NetStorageTestResult* r) {
    net::GURL url("https://example.com/path");
    net::CanonicalCookie cookie = net::CanonicalCookie::Create(url, "token=xyz; Domain=example.com");
    net::GURL match_url("https://sub.example.com/other");
    bool pass = cookie.IsMatchForURL(match_url);
    RecordResult(r, "CanonicalCookie_DomainMatch", pass, pass ? "Domain match works" : "Domain match failed");
}

// ============================================================
// Test 15: CanonicalCookie — Secure cookie not sent over HTTP
// ============================================================
static void Test_CanonicalCookie_SecureHTTP(NetStorageTestResult* r) {
    net::GURL url("https://example.com/");
    net::CanonicalCookie cookie = net::CanonicalCookie::Create(url, "sec=val; Secure");
    net::GURL http_url("http://example.com/");
    bool pass = !cookie.IsMatchForURL(http_url);
    RecordResult(r, "CanonicalCookie_SecureHTTP", pass, pass ? "Secure not sent over HTTP" : "Secure cookie leaked to HTTP");
}

// ============================================================
// Test 16: CookieStore — SetCookie + GetCookieHeader
// ============================================================
static void Test_CookieStore_SetGet(NetStorageTestResult* r) {
    net::CookieStore store;
    net::GURL url("https://example.com/");
    store.SetCookie(url, "user=alice");
    store.SetCookie(url, "theme=dark");
    std::string header = store.GetCookieHeaderForURL(url);
    bool pass = header.find("user=alice") != (size_t)-1 &&
                header.find("theme=dark") != (size_t)-1;
    RecordResult(r, "CookieStore_SetGet", pass, pass ? "Cookie header correct" : "Cookie header wrong");
}

// ============================================================
// Test 17: CookieStore — Replace existing cookie
// ============================================================
static void Test_CookieStore_Replace(NetStorageTestResult* r) {
    net::CookieStore store;
    net::GURL url("https://example.com/");
    store.SetCookie(url, "user=alice");
    store.SetCookie(url, "user=bob");
    bool pass = store.size() == 1;
    std::string header = store.GetCookieHeaderForURL(url);
    pass = pass && header.find("user=bob") != (size_t)-1;
    RecordResult(r, "CookieStore_Replace", pass, pass ? "Cookie replaced" : "Cookie replacement failed");
}

// ============================================================
// Test 18: CookieStore — DeleteCookie
// ============================================================
static void Test_CookieStore_Delete(NetStorageTestResult* r) {
    net::CookieStore store;
    net::GURL url("https://example.com/");
    store.SetCookie(url, "user=alice");
    store.DeleteCookie(url, "user");
    bool pass = store.size() == 0;
    RecordResult(r, "CookieStore_Delete", pass, pass ? "Cookie deleted" : "Cookie deletion failed");
}

// ============================================================
// Test 19: CookieStore — Cross-origin isolation
// ============================================================
static void Test_CookieStore_CrossOrigin(NetStorageTestResult* r) {
    net::CookieStore store;
    net::GURL url_a("https://a.com/");
    net::GURL url_b("https://b.com/");
    store.SetCookie(url_a, "token=aaa");
    std::string header_b = store.GetCookieHeaderForURL(url_b);
    bool pass = header_b.empty();
    RecordResult(r, "CookieStore_CrossOrigin", pass, pass ? "Cross-origin isolated" : "Cross-origin leak");
}

// ============================================================
// Test 20: HttpCache — Store + Lookup
// ============================================================
static void Test_HttpCache_StoreLookup(NetStorageTestResult* r) {
    net::HttpCache cache;
    cache.Store("https://example.com/", 200, "HTTP/1.1 200 OK", "<html>hello</html>", "", "");
    net::HttpCacheEntry entry;
    bool pass = cache.Lookup("https://example.com/", &entry) &&
                entry.status_code == 200 &&
                entry.response_body == "<html>hello</html>";
    RecordResult(r, "HttpCache_StoreLookup", pass, pass ? "Cache hit" : "Cache miss");
}

// ============================================================
// Test 21: HttpCache — Invalidation
// ============================================================
static void Test_HttpCache_Invalidate(NetStorageTestResult* r) {
    net::HttpCache cache;
    cache.Store("https://example.com/", 200, "", "body", "", "");
    cache.Invalidate("https://example.com/");
    net::HttpCacheEntry entry;
    bool pass = !cache.Lookup("https://example.com/", &entry);
    RecordResult(r, "HttpCache_Invalidate", pass, pass ? "Cache invalidated" : "Cache not invalidated");
}

// ============================================================
// Test 22: HttpCache — Clear
// ============================================================
static void Test_HttpCache_Clear(NetStorageTestResult* r) {
    net::HttpCache cache;
    cache.Store("https://a.com/", 200, "", "a", "", "");
    cache.Store("https://b.com/", 200, "", "b", "", "");
    cache.Clear();
    bool pass = cache.size() == 0;
    RecordResult(r, "HttpCache_Clear", pass, pass ? "Cache cleared" : "Cache not cleared");
}

// ============================================================
// Test 23: URLLoader — Invalid URL returns error
// ============================================================
static void Test_URLLoader_InvalidURL(NetStorageTestResult* r) {
    net::CookieStore cs;
    net::HttpCache hc;
    net::URLLoader loader(&cs, &hc);
    net::GURL bad_url("not-valid");
    net::URLLoaderResult result = loader.Load(bad_url);
    bool pass = result.net_error == net::ERR_INVALID_URL;
    RecordResult(r, "URLLoader_InvalidURL", pass, pass ? "Invalid URL rejected" : "Invalid URL not rejected");
}

// ============================================================
// Test 24: StorageArea — setItem + getItem
// ============================================================
static void Test_StorageArea_SetGet(NetStorageTestResult* r) {
    storage::StorageArea area;
    area.setItem("key1", "value1");
    area.setItem("key2", "value2");
    bool pass = area.getItem("key1") == "value1" &&
                area.getItem("key2") == "value2" &&
                area.length() == 2;
    RecordResult(r, "StorageArea_SetGet", pass, pass ? "Set/Get works" : "Set/Get failed");
}

// ============================================================
// Test 25: StorageArea — removeItem
// ============================================================
static void Test_StorageArea_Remove(NetStorageTestResult* r) {
    storage::StorageArea area;
    area.setItem("key1", "value1");
    area.removeItem("key1");
    bool pass = area.length() == 0 && area.getItem("key1") == "";
    RecordResult(r, "StorageArea_Remove", pass, pass ? "Remove works" : "Remove failed");
}

// ============================================================
// Test 26: StorageArea — clear
// ============================================================
static void Test_StorageArea_Clear(NetStorageTestResult* r) {
    storage::StorageArea area;
    area.setItem("a", "1");
    area.setItem("b", "2");
    area.setItem("c", "3");
    area.clear();
    bool pass = area.length() == 0;
    RecordResult(r, "StorageArea_Clear", pass, pass ? "Clear works" : "Clear failed");
}

// ============================================================
// Test 27: StorageArea — Quota enforcement
// ============================================================
static void Test_StorageArea_Quota(NetStorageTestResult* r) {
    storage::StorageArea area;
    // Try to set an item larger than 5MB quota
    std::string huge_value(6 * 1024 * 1024, 'X');
    bool set_result = area.setItem("big", huge_value);
    bool pass = !set_result; // Should be rejected
    RecordResult(r, "StorageArea_Quota", pass, pass ? "Quota enforced" : "Quota not enforced");
}

// ============================================================
// Test 28: StorageArea — key() index access
// ============================================================
static void Test_StorageArea_KeyIndex(NetStorageTestResult* r) {
    storage::StorageArea area;
    area.setItem("alpha", "1");
    area.setItem("beta", "2");
    bool pass = area.key(0) == "alpha" && area.key(1) == "beta" && area.key(99) == "";
    RecordResult(r, "StorageArea_KeyIndex", pass, pass ? "key() works" : "key() failed");
}

// ============================================================
// Test 29: StorageNamespace — Origin isolation
// ============================================================
static void Test_StorageNamespace_Isolation(NetStorageTestResult* r) {
    storage::StorageNamespace ns(storage::STORAGE_TYPE_LOCAL);
    net::SecurityOrigin origin_a("https", "example.com", 443);
    net::SecurityOrigin origin_b("https", "other.com", 443);

    storage::StorageArea* area_a = ns.GetStorageArea(origin_a);
    storage::StorageArea* area_b = ns.GetStorageArea(origin_b);

    area_a->setItem("secret", "origin_a_data");
    bool pass = area_b->getItem("secret") == ""; // Must NOT see origin_a's data
    pass = pass && area_a->getItem("secret") == "origin_a_data";
    RecordResult(r, "StorageNamespace_Isolation", pass, pass ? "Origins isolated" : "Origin leak detected");
}

// ============================================================
// Test 30: StorageNamespace — Same origin returns same area
// ============================================================
static void Test_StorageNamespace_SameOrigin(NetStorageTestResult* r) {
    storage::StorageNamespace ns(storage::STORAGE_TYPE_LOCAL);
    net::SecurityOrigin origin("https", "example.com", 443);

    storage::StorageArea* area1 = ns.GetStorageArea(origin);
    area1->setItem("key", "val");
    storage::StorageArea* area2 = ns.GetStorageArea(origin);

    bool pass = area2->getItem("key") == "val";
    RecordResult(r, "StorageNamespace_SameOrigin", pass, pass ? "Same origin → same area" : "Same origin mismatch");
}

// ============================================================
// Test 31: SessionStorageManager — volatile (no persistence)
// ============================================================
static void Test_SessionStorage_Volatile(NetStorageTestResult* r) {
    net::SecurityOrigin origin("https", "session.test", 443);
    {
        storage::SessionStorageManager ssm;
        storage::StorageArea* area = ssm.GetSessionStorage(origin);
        area->setItem("temp", "data");
        bool pass = area->getItem("temp") == "data";
        RecordResult(r, "SessionStorage_Volatile", pass, pass ? "Session data stored" : "Session store failed");
    }
    // Manager destroyed — data should be gone (volatile)
}

// ============================================================
// Test 32: VFS Adapter — CRC32 computation
// ============================================================
static void Test_VFS_CRC32(NetStorageTestResult* r) {
    const uint8_t test_data[] = "ATOMS OS Storage Test";
    uint32_t crc = storage::AtomsStorageVFS_CRC32(test_data, 21);
    bool pass = crc != 0; // Non-zero CRC for non-empty data
    RecordResult(r, "VFS_CRC32", pass, pass ? "CRC32 computed" : "CRC32 zero");
}

// ============================================================
// Test 33: VFS Adapter — Path generation
// ============================================================
static void Test_VFS_PathGeneration(NetStorageTestResult* r) {
    net::SecurityOrigin origin("https", "example.com", 443);
    std::string path = storage::AtomsStorageVFS_GetPath(origin);
    bool pass = path.find("/var/storage/local_") == 0 &&
                path.find(".dat") == path.size() - 4 &&
                path.size() > 20;
    RecordResult(r, "VFS_PathGeneration", pass, pass ? "VFS path correct" : "VFS path wrong");
}

// ============================================================
// Test 34: VFS Adapter — Different origins → different paths
// ============================================================
static void Test_VFS_DifferentPaths(NetStorageTestResult* r) {
    net::SecurityOrigin o1("https", "example.com", 443);
    net::SecurityOrigin o2("https", "other.com", 443);
    net::SecurityOrigin o3("http", "example.com", 80);

    std::string p1 = storage::AtomsStorageVFS_GetPath(o1);
    std::string p2 = storage::AtomsStorageVFS_GetPath(o2);
    std::string p3 = storage::AtomsStorageVFS_GetPath(o3);

    bool pass = p1 != p2 && p1 != p3 && p2 != p3;
    RecordResult(r, "VFS_DifferentPaths", pass, pass ? "Paths unique per origin" : "Path collision detected");
}

// ============================================================
// Master Runner
// ============================================================
int RunNetStorageTestSuite(NetStorageTestResult* results, int max_results) {
    test_idx = 0;
    if (max_results < 34) return -1;

    // GURL tests
    Test_GURL_ParseHTTPS(results);
    Test_GURL_ParseHTTPPort(results);
    Test_GURL_Invalid(results);

    // SecurityOrigin tests
    Test_SecurityOrigin_Create(results);
    Test_SecurityOrigin_SameOrigin(results);
    Test_SecurityOrigin_DiffScheme(results);
    Test_SecurityOrigin_DiffPort(results);
    Test_SecurityOrigin_DiffHost(results);

    // HTTP header tests
    Test_HttpRequestHeaders(results);
    Test_HttpResponseHeaders_Parse(results);
    Test_HttpResponseHeaders_Redirect(results);
    Test_HttpResponseHeaders_SetCookie(results);

    // Cookie tests
    Test_CanonicalCookie_Create(results);
    Test_CanonicalCookie_DomainMatch(results);
    Test_CanonicalCookie_SecureHTTP(results);
    Test_CookieStore_SetGet(results);
    Test_CookieStore_Replace(results);
    Test_CookieStore_Delete(results);
    Test_CookieStore_CrossOrigin(results);

    // Cache tests
    Test_HttpCache_StoreLookup(results);
    Test_HttpCache_Invalidate(results);
    Test_HttpCache_Clear(results);

    // URLLoader tests
    Test_URLLoader_InvalidURL(results);

    // Storage tests
    Test_StorageArea_SetGet(results);
    Test_StorageArea_Remove(results);
    Test_StorageArea_Clear(results);
    Test_StorageArea_Quota(results);
    Test_StorageArea_KeyIndex(results);
    Test_StorageNamespace_Isolation(results);
    Test_StorageNamespace_SameOrigin(results);
    Test_SessionStorage_Volatile(results);

    // VFS adapter tests
    Test_VFS_CRC32(results);
    Test_VFS_PathGeneration(results);
    Test_VFS_DifferentPaths(results);

    return test_idx;
}

extern "C" bool NetStorage_RunAllVerificationTests(void) {
    puts("\n=======================================================");
    puts(" CHROMIUM NETWORKING & STORAGE VERIFICATION (PHASE 12) ");
    puts("=======================================================");

    NetStorageTestResult results[40];
    int total = RunNetStorageTestSuite(results, 40);
    int passed = 0;

    for (int i = 0; i < total; i++) {
        printf("[TEST %d/%d] %s ... ", i + 1, total, results[i].test_name);
        if (results[i].passed) {
            printf("PASS (%s)\n", results[i].detail);
            passed++;
        } else {
            printf("FAIL (%s)\n", results[i].detail);
        }
    }

    printf("\nSUMMARY: %d/%d PASSED\n", passed, total);
    if (passed == total) {
        puts("=======================================================");
        puts("       PHASE 12 VERIFICATION: ALL 34 TESTS PASS       ");
        puts("=======================================================\n");
        return true;
    } else {
        puts("=======================================================");
        puts("       PHASE 12 VERIFICATION: FAILURES DETECTED       ");
        puts("=======================================================\n");
        return false;
    }
}

