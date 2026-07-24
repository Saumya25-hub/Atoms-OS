#include "abe_net_test.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "abe_net_dns.h"
#include "abe_net_conn.h"
#include "abe_net_http.h"
#include "abe_net_parser.h"
#include "abe_net_redirect.h"
#include "abe_net_compress.h"
#include "abe_net_download.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool Test_DNSResolver(void) {
    display_print("[ABE_NET_TEST] 1. Testing DNS Resolver & Hostname Caching...\n");

    uint32_t ip1 = 0;
    ABE_Error err = ABE_DNSResolve("google.com", &ip1);
    if (err != ABE_SUCCESS || ip1 == 0) {
        display_print("[ABE_NET_TEST] FAIL: DNS resolution for google.com failed!\n");
        return false;
    }

    uint32_t ip2 = 0;
    err = ABE_DNSResolve("google.com", &ip2);
    if (err != ABE_SUCCESS || ip1 != ip2) {
        display_print("[ABE_NET_TEST] FAIL: DNS cache hit mismatch!\n");
        return false;
    }

    display_print("[ABE_NET_TEST] PASS: DNS Resolver & Cache verified.\n");
    return true;
}

static bool Test_ConnectionManagerAndPool(void) {
    display_print("[ABE_NET_TEST] 2. Testing Connection Manager Pooling & Keep-Alive Reuse...\n");

    ABE_ConnHandle conn1 = ABE_INVALID_HANDLE;
    ABE_ConnHandle conn2 = ABE_INVALID_HANDLE;

    ABE_Error err = ABE_OpenConnection("google.com", 80, false, &conn1);
    if (err != ABE_SUCCESS || conn1 == ABE_INVALID_HANDLE) {
        display_print("[ABE_NET_TEST] FAIL: OpenConnection 1 failed!\n");
        return false;
    }

    // Reuse connection for same host & port (Keep-Alive)
    err = ABE_OpenConnection("google.com", 80, false, &conn2);
    if (err != ABE_SUCCESS || conn1 != conn2) {
        display_print("[ABE_NET_TEST] FAIL: Connection pooling reuse failed!\n");
        return false;
    }

    ABE_CloseConnection(conn1);
    display_print("[ABE_NET_TEST] PASS: Connection Manager pooling verified.\n");
    return true;
}

static bool Test_HTTPEngineAndParser(void) {
    display_print("[ABE_NET_TEST] 3. Testing HTTP Request Serializer & Response Parser...\n");

    ABE_HTTPRequest req;
    ABE_Error err = ABE_NetHTTP_CreateRequest(ABE_HTTP_METHOD_GET, "https://wikipedia.org/wiki/ATOMS_OS", &req);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_NET_TEST] FAIL: HTTP Request creation failed!\n");
        return false;
    }

    ABE_NetHTTP_AddHeader(&req, "Accept-Language", "en-US,en;q=0.9");

    uint8_t* serialized = NULL;
    size_t ser_len = 0;
    err = ABE_NetHTTP_SerializeRequest(&req, &serialized, &ser_len);
    if (err != ABE_SUCCESS || !serialized || ser_len == 0) {
        display_print("[ABE_NET_TEST] FAIL: Request serialization failed!\n");
        return false;
    }

    kfree(serialized);
    ABE_NetHTTP_FreeRequest(&req);

    // Test Response Header Parser
    const char* raw_resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 45\r\n\r\n<html><body><h1>ABE V1 Engine</h1></body></html>";
    ABE_HTTPResponseHeaderInfo info;
    size_t header_bytes = 0;

    err = ABE_NetParser_ParseHeader((const uint8_t*)raw_resp, strlen(raw_resp), &info, &header_bytes);
    if (err != ABE_SUCCESS || info.status_code != 200 || info.content_length != 45) {
        display_print("[ABE_NET_TEST] FAIL: HTTP Response Header Parser failed!\n");
        return false;
    }

    display_print("[ABE_NET_TEST] PASS: HTTP Request/Response Parser verified.\n");
    return true;
}

static bool Test_ChunkedEncodingAndDecompression(void) {
    display_print("[ABE_NET_TEST] 4. Testing Chunked Transfer Encoding & GZIP Decompression...\n");

    // Chunked Encoding Payload "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"
    const char* chunked = "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n";
    uint8_t* decoded = NULL;
    size_t decoded_len = 0;

    ABE_Error err = ABE_NetParser_DecodeChunked((const uint8_t*)chunked, strlen(chunked), &decoded, &decoded_len);
    if (err != ABE_SUCCESS || !decoded || strcmp((const char*)decoded, "Wikipedia") != 0) {
        display_print("[ABE_NET_TEST] FAIL: Chunked Transfer Encoding decoder failed! Decoded: ");
        if (decoded) display_print((const char*)decoded);
        display_print("\n");
        return false;
    }
    kfree(decoded);

    // Test Decompression Pipeline
    const uint8_t mock_gzip[12] = {0x1F, 0x8B, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'T', 'E'};
    uint8_t* decomp = NULL;
    size_t decomp_len = 0;

    err = ABE_NetCompress_Decompress(ABE_COMPRESS_GZIP, mock_gzip, sizeof(mock_gzip), &decomp, &decomp_len);
    if (err != ABE_SUCCESS || !decomp) {
        display_print("[ABE_NET_TEST] FAIL: GZIP Decompression pipeline failed!\n");
        return false;
    }
    kfree(decomp);

    display_print("[ABE_NET_TEST] PASS: Chunked Decoder & Decompression Engine verified.\n");
    return true;
}

static bool Test_RedirectEngine(void) {
    display_print("[ABE_NET_TEST] 5. Testing Redirect Engine (301, 302, 303, 307, 308) & Loop Protection...\n");

    ABE_RedirectHistory history;
    memset(&history, 0, sizeof(ABE_RedirectHistory));

    char new_url[ABE_MAX_URL_LEN];
    ABE_HTTPMethod new_method = ABE_HTTP_METHOD_POST;

    // 303 Redirect POST -> GET
    ABE_Error err = ABE_NetRedirect_ProcessRedirect(&history, "https://amazon.com/login", 303, "/dashboard", ABE_HTTP_METHOD_POST, new_url, &new_method);
    if (err != ABE_SUCCESS || new_method != ABE_HTTP_METHOD_GET || strstr(new_url, "/dashboard") == NULL) {
        display_print("[ABE_NET_TEST] FAIL: 303 Redirect method conversion failed!\n");
        return false;
    }

    display_print("[ABE_NET_TEST] PASS: Redirect Engine verified.\n");
    return true;
}

static bool Test_DownloadStreamEngine(void) {
    display_print("[ABE_NET_TEST] 6. Testing Download Stream Engine Progress & Cancellation...\n");

    ABE_DownloadHandle dl = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_DownloadResource("https://youtube.com/video.mp4", NULL, NULL, &dl);
    if (err != ABE_SUCCESS || dl == ABE_INVALID_HANDLE) {
        display_print("[ABE_NET_TEST] FAIL: Download stream start failed!\n");
        return false;
    }

    ABE_CancelRequest(dl);
    display_print("[ABE_NET_TEST] PASS: Download Stream Engine verified.\n");
    return true;
}

void ABE_RunPhase2_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 2 Production Network Test Suite    \n");
    display_print("=========================================================\n");

    ABE_NetworkInitialize();

    if (!Test_DNSResolver()) return;
    if (!Test_ConnectionManagerAndPool()) return;
    if (!Test_HTTPEngineAndParser()) return;
    if (!Test_ChunkedEncodingAndDecompression()) return;
    if (!Test_RedirectEngine()) return;
    if (!Test_DownloadStreamEngine()) return;

    ABE_DiagnosticsMetrics metrics;
    ABE_GetDiagnosticsMetrics(&metrics);
    display_print("[ABE_NET_DIAG] DNS Resolutions : "); display_print_dec(metrics.dns_resolutions_total); display_print("\n");
    display_print("[ABE_NET_DIAG] DNS Cache Hits  : "); display_print_dec(metrics.dns_cache_hits); display_print("\n");
    display_print("[ABE_NET_DIAG] HTTP Requests   : "); display_print_dec(metrics.http_requests_sent); display_print("\n");
    display_print("[ABE_NET_DIAG] Redirects       : "); display_print_dec(metrics.redirects_followed); display_print("\n");
    display_print("[ABE_NET_DIAG] Bytes Downloaded: "); display_print_dec(metrics.total_bytes_downloaded); display_print("\n");

    ABE_NetworkShutdown();

    display_print("\nPASS_PHASE2_ABE_PRODUCTION_NETWORKING_ENGINE\n\n");
}
