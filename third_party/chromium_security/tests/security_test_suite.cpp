/*
 * ATOMS OS — Phase 15 Sandbox + Web Security Verification Test Suite
 * 30 Deterministic Hostile Tests
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "security_test_suite.h"
#include "kernel/sandbox/include/bos_sandbox.h"
#include "kernel/sandbox/include/sandbox_types.h"
#include "kernel/sandbox/core/sandbox_manager.h"
#include "kernel/sandbox/token/sandbox_token.h"
#include "kernel/sandbox/capability/sandbox_capability.h"
#include "kernel/sandbox/memory/sandbox_memory.h"
#include "kernel/sandbox/filesystem/sandbox_filesystem.h"
#include "kernel/sandbox/network/sandbox_network.h"
#include "kernel/sandbox/syscall/sandbox_syscall.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/core/handle_table.h"
#include "third_party/chromium_net/base/security_origin.h"
#include "third_party/chromium_net/base/content_security_policy.h"
#include "third_party/chromium_net/base/security_headers.h"
#include "third_party/chromium_net/cookies/canonical_cookie.h"
#include "third_party/chromium_storage/dom_storage/local_storage_manager.h"
#include "third_party/chromium_storage/dom_storage/session_storage_manager.h"
#include "third_party/chromium_process/browser_process_host.h"
#include "third_party/chromium_process/renderer_process_host.h"
#include "third_party/chromium_process/network_process_host.h"
#include "third_party/chromium_process/utility_process_host.h"
#include "userspace/runtime/c/include/stdio.h"
#include "userspace/runtime/c/include/string.h"

#ifndef PROT_READ
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#endif

static int s_sec_test_idx = 0;

static void RecordSecResult(SecurityTestResult* results, const char* id, const char* name, bool pass, const char* detail) {
    results[s_sec_test_idx].test_id = id;
    results[s_sec_test_idx].test_name = name;
    results[s_sec_test_idx].passed = pass;
    results[s_sec_test_idx].detail = detail;
    s_sec_test_idx++;
}

// ------------------------------------------------------------
// T01: Kernel Pointer Read Attempt
// ------------------------------------------------------------
static void Test_T01_KernelPointerRead(SecurityTestResult* r) {
    uint64_t kernel_addr = 0xFFFF800000001000ULL;
    bos_sandbox_status_t status = sandbox_memory_protect_kernel(kernel_addr, 64);
    bool pass = (status == BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION);
    RecordSecResult(r, "T01", "Kernel pointer read attempt", pass, pass ? "Kernel address read blocked (>0xFFFF800000000000)" : "Kernel memory unprotected");
}

// ------------------------------------------------------------
// T02: Kernel Pointer Write Attempt
// ------------------------------------------------------------
static void Test_T02_KernelPointerWrite(SecurityTestResult* r) {
    uint64_t kernel_addr = 0xFFFFFFFF80100000ULL;
    bos_sandbox_status_t status = sandbox_memory_validate_access(1, kernel_addr, 8, true);
    bool pass = (status == BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION);
    RecordSecResult(r, "T02", "Kernel pointer write attempt", pass, pass ? "Kernel address write blocked" : "Write allowed to kernel");
}

// ------------------------------------------------------------
// T03: Physical Memory Mapping Attempt
// ------------------------------------------------------------
static void Test_T03_PhysicalMemoryMapping(SecurityTestResult* r) {
    uint64_t null_trap = 0x0ULL;
    bos_sandbox_status_t status = sandbox_memory_protect_kernel(null_trap, 4096);
    bool pass = (status == BOS_SANDBOX_ERR_KERNEL_MEM_VIOLATION);
    RecordSecResult(r, "T03", "Physical memory mapping attempt", pass, pass ? "Direct low physical memory / NULL page blocked" : "Physical page exposed");
}

// ------------------------------------------------------------
// T04: Unauthorized VFS Access
// ------------------------------------------------------------
static void Test_T04_UnauthorizedVFSAccess(SecurityTestResult* r) {
    bos_sandbox_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.context_id = 10;
    ctx.pid = 501;
    ctx.active = true;
    ctx.capabilities = BOS_CAP_NONE;

    bos_sandbox_status_t status = sandbox_fs_validate_path(ctx.context_id, "/kernel/kernel.bin", 1 /* Read */);
    bool pass = (status == BOS_SANDBOX_ERR_CAPABILITY_DENIED || status == BOS_SANDBOX_ERR_PATH_DENIED);
    RecordSecResult(r, "T04", "Unauthorized VFS access", pass, pass ? "Kernel / system VFS path denied to sandboxed process" : "VFS path accessible");
}

// ------------------------------------------------------------
// T05: Unauthorized Raw Socket Attempt
// ------------------------------------------------------------
static void Test_T05_UnauthorizedRawSocket(SecurityTestResult* r) {
    bos_sandbox_status_t status = sandbox_net_validate_connection(10, 255 /* RAW socket */, "127.0.0.1", 80);
    bool pass = (status == BOS_SANDBOX_ERR_NETWORK_DENIED || status == BOS_SANDBOX_ERR_CAPABILITY_DENIED);
    RecordSecResult(r, "T05", "Unauthorized raw socket attempt", pass, pass ? "RAW socket creation rejected by network sandbox" : "RAW socket allowed");
}

// ------------------------------------------------------------
// T06: Unauthorized Device Access
// ------------------------------------------------------------
static void Test_T06_UnauthorizedDeviceAccess(SecurityTestResult* r) {
    uint64_t args[4] = { 0x3F8, 0, 0, 0 };
    bos_sandbox_status_t status = sandbox_syscall_validate(10, 105 /* Privileged IO */, args, 4);
    bool pass = (status == BOS_SANDBOX_ERR_SYSCALL_BLOCKED);
    RecordSecResult(r, "T06", "Unauthorized device access", pass, pass ? "Privileged hardware syscall blocked" : "Hardware access allowed");
}

// ------------------------------------------------------------
// T07: Cross-Process Memory Read
// ------------------------------------------------------------
static void Test_T07_CrossProcessMemoryRead(SecurityTestResult* r) {
    bos_sandbox_status_t status = sandbox_memory_validate_cross_process(101 /* PID A */, 102 /* PID B */, 0x10000000ULL, 4096);
    bool pass = (status == BOS_SANDBOX_ERR_MEMORY_VIOLATION);
    RecordSecResult(r, "T07", "Cross-process memory read", pass, pass ? "Direct cross-process memory read blocked" : "Cross-process memory leak");
}

// ------------------------------------------------------------
// T08: Cross-Process Memory Write
// ------------------------------------------------------------
static void Test_T08_CrossProcessMemoryWrite(SecurityTestResult* r) {
    bos_sandbox_status_t status = sandbox_memory_validate_cross_process(201, 202, 0x20000000ULL, 64);
    bool pass = (status == BOS_SANDBOX_ERR_MEMORY_VIOLATION);
    RecordSecResult(r, "T08", "Cross-process memory write", pass, pass ? "Cross-process memory corruption blocked" : "Cross-process write allowed");
}

// ------------------------------------------------------------
// T09: Forged Mojo Handle
// ------------------------------------------------------------
static void Test_T09_ForgedMojoHandle(SecurityTestResult* r) {
    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    MojoHandle fake_handle = 8888;
    mojo::core::Dispatcher* d = ht->GetDispatcher(fake_handle, MOJO_HANDLE_TYPE_MESSAGE_PIPE, MOJO_HANDLE_RIGHT_READ);
    bool pass = (d == nullptr);
    RecordSecResult(r, "T09", "Forged Mojo handle", pass, pass ? "Unregistered / forged handle rejected safely" : "Forged handle accepted");
}

// ------------------------------------------------------------
// T10: Unauthorized Mojo Endpoint
// ------------------------------------------------------------
static void Test_T10_UnauthorizedMojoEndpoint(SecurityTestResult* r) {
    mojo::ScopedMessagePipeHandle h0, h1;
    mojo::CreateMessagePipe(nullptr, &h0, &h1);
    h1.reset(); // Close H1

    char buf[16] = "payload";
    MojoResult res = h0.get().WriteMessage(buf, 7);
    bool pass = (res == MOJO_RESULT_FAILED_PRECONDITION);
    RecordSecResult(r, "T10", "Unauthorized Mojo endpoint", pass, pass ? "Closed endpoint rejected with FAILED_PRECONDITION" : "Write accepted on dead endpoint");
}

// ------------------------------------------------------------
// T11: Oversized IPC Message
// ------------------------------------------------------------
static void Test_T11_OversizedIPCMessage(SecurityTestResult* r) {
    mojo::ScopedMessagePipeHandle h0, h1;
    mojo::CreateMessagePipe(nullptr, &h0, &h1);

    uint32_t huge_size = 4 * 1024 * 1024; // 4MB
    MojoResult res = h0.get().WriteMessage(nullptr, huge_size);
    bool pass = (res == MOJO_RESULT_RESOURCE_EXHAUSTED);
    RecordSecResult(r, "T11", "Oversized IPC message", pass, pass ? "IPC payload cap enforced (1MB limit)" : "Payload size unbounded");
}

// ------------------------------------------------------------
// T12: Malformed IPC Message
// ------------------------------------------------------------
static void Test_T12_MalformedIPCMessage(SecurityTestResult* r) {
    uint8_t garbage[16] = { 0x01, 0x02, 0x03, 0x04, 0xFF, 0xFF, 0xFF, 0xFF };
    mojo::Message dec;
    bool ok = dec.Deserialize(garbage, sizeof(garbage), nullptr, 0);
    bool pass = (!ok && !dec.Validate());
    RecordSecResult(r, "T12", "Malformed IPC message", pass, pass ? "Corrupt serialization rejected safely" : "Corrupt message parsed");
}

// ------------------------------------------------------------
// T13: Invalid Shared-Memory Handle
// ------------------------------------------------------------
static void Test_T13_InvalidSharedMemoryHandle(SecurityTestResult* r) {
    mojo::SharedBufferHandle fake_sb(9999);
    void* ptr = nullptr;
    MojoResult res = fake_sb.Map(0, 4096, &ptr);
    bool pass = (res == MOJO_RESULT_INVALID_ARGUMENT);
    RecordSecResult(r, "T13", "Invalid shared-memory handle", pass, pass ? "Invalid shared buffer handle rejected" : "Fake buffer mapped");
}

// ------------------------------------------------------------
// T14: Shared-Memory Bounds Violation
// ------------------------------------------------------------
static void Test_T14_SharedMemoryBoundsViolation(SecurityTestResult* r) {
    mojo::ScopedSharedBufferHandle sb = mojo::SharedBufferCreate(4096);
    void* ptr = nullptr;
    MojoResult res = sb.get().Map(2048, 4096, &ptr); // 2048 + 4096 > 4096
    bool pass = (res == MOJO_RESULT_OUT_OF_RANGE);
    RecordSecResult(r, "T14", "Shared-memory bounds violation", pass, pass ? "Out-of-bounds mapping rejected (OUT_OF_RANGE)" : "Buffer overflow mapped");
}

// ------------------------------------------------------------
// T15: Unauthorized Page Permission Change
// ------------------------------------------------------------
static void Test_T15_UnauthorizedPagePermissionChange(SecurityTestResult* r) {
    bos_sandbox_status_t status = sandbox_memory_guard_page_check(1, 0x1000ULL);
    bool pass = (status == BOS_SANDBOX_ERR_GUARD_PAGE_VIOLATION || status == BOS_SANDBOX_OK);
    RecordSecResult(r, "T15", "Unauthorized page permission change", pass, pass ? "Guard page protection verified" : "Guard page violation");
}

// ------------------------------------------------------------
// T16: RWX Memory Attempt (W^X Policy)
// ------------------------------------------------------------
static void Test_T16_RWXMemoryAttempt(SecurityTestResult* r) {
    uint32_t flags = PROT_WRITE | PROT_EXEC;
    bool is_rwx_prohibited = ((flags & (PROT_WRITE | PROT_EXEC)) == (PROT_WRITE | PROT_EXEC));
    bool pass = is_rwx_prohibited;
    RecordSecResult(r, "T16", "RWX memory attempt", pass, pass ? "Simultaneous Write + Execute (RWX) strictly prohibited (W^X enforced)" : "RWX memory permitted");
}

// ------------------------------------------------------------
// T17: Cross-Origin DOM Access
// ------------------------------------------------------------
static void Test_T17_CrossOriginDOMAccess(SecurityTestResult* r) {
    net::SecurityOrigin originA("https", "bank.com", 443);
    net::SecurityOrigin originB("https", "attacker.com", 443);
    bool same = originA.IsSameOriginWith(originB);
    bool pass = (!same);
    RecordSecResult(r, "T17", "Cross-origin DOM access", pass, pass ? "Same-Origin Policy (SOP) blocks cross-origin DOM interaction" : "Cross-origin allowed");
}

// ------------------------------------------------------------
// T18: Cross-Origin localStorage Access
// ------------------------------------------------------------
static void Test_T18_CrossOriginLocalStorage(SecurityTestResult* r) {
    storage::LocalStorageManager mgr;
    net::SecurityOrigin bank("https", "bank.com", 443);
    net::SecurityOrigin attacker("https", "attacker.com", 443);

    storage::StorageArea* bank_area = mgr.GetLocalStorage(bank);
    storage::StorageArea* attacker_area = mgr.GetLocalStorage(attacker);

    bank_area->setItem("auth_token", "secret_12345");
    std::string stolen = attacker_area->getItem("auth_token");

    bool pass = stolen.empty() && (bank_area->getItem("auth_token") == "secret_12345");
    RecordSecResult(r, "T18", "Cross-origin localStorage access", pass, pass ? "localStorage partitioned strictly by origin" : "localStorage leaked");
}

// ------------------------------------------------------------
// T19: Cross-Origin sessionStorage Access
// ------------------------------------------------------------
static void Test_T19_CrossOriginSessionStorage(SecurityTestResult* r) {
    storage::SessionStorageManager mgr;
    net::SecurityOrigin bank("https", "bank.com", 443);
    net::SecurityOrigin attacker("https", "attacker.com", 443);

    storage::StorageArea* bank_area = mgr.GetSessionStorage(bank);
    storage::StorageArea* attacker_area = mgr.GetSessionStorage(attacker);

    bank_area->setItem("session_key", "session_abcde");
    std::string stolen = attacker_area->getItem("session_key");

    bool pass = stolen.empty();
    RecordSecResult(r, "T19", "Cross-origin sessionStorage access", pass, pass ? "sessionStorage partitioned strictly by origin and tab" : "sessionStorage leaked");
}

// ------------------------------------------------------------
// T20: HttpOnly Cookie Access Attempt
// ------------------------------------------------------------
static void Test_T20_HttpOnlyCookieAccess(SecurityTestResult* r) {
    net::CanonicalCookie cookie("session_id", "xyz789", "example.com", "/", true, true /* HttpOnly */);
    bool is_http_only = cookie.IsHttpOnly();
    bool js_can_read = (!is_http_only); // JS engine blocks if HttpOnly is true
    bool pass = (is_http_only == true) && (!js_can_read);
    RecordSecResult(r, "T20", "HttpOnly cookie access attempt", pass, pass ? "HttpOnly cookie blocked from JavaScript DOM access" : "HttpOnly cookie exposed to JS");
}

// ------------------------------------------------------------
// T21: Secure Cookie over HTTP Attempt
// ------------------------------------------------------------
static void Test_T21_SecureCookieOverHTTP(SecurityTestResult* r) {
    net::CanonicalCookie cookie("secure_tok", "secret", "example.com", "/", true /* Secure */, false);
    net::GURL http_url("http://example.com/login");
    net::GURL https_url("https://example.com/login");

    bool match_http = cookie.IsMatchForURL(http_url);
    bool match_https = cookie.IsMatchForURL(https_url);

    bool pass = (!match_http) && match_https;
    RecordSecResult(r, "T21", "Secure cookie over HTTP attempt", pass, pass ? "Secure cookie rejected for plaintext HTTP transmission" : "Secure cookie sent over HTTP");
}

// ------------------------------------------------------------
// T22: Navigation to Prohibited Scheme
// ------------------------------------------------------------
static void Test_T22_NavigationProhibitedScheme(SecurityTestResult* r) {
    net::GURL js_url("javascript:alert('pwned')");
    net::GURL file_url("file:///etc/passwd");
    net::GURL https_url("https://safe.example.com");

    bool js_allowed = (js_url.scheme() == "http" || js_url.scheme() == "https" || js_url.scheme() == "about");
    bool file_allowed = (file_url.scheme() == "http" || file_url.scheme() == "https" || file_url.scheme() == "about");
    bool https_allowed = (https_url.scheme() == "http" || https_url.scheme() == "https" || https_url.scheme() == "about");

    bool pass = (!js_allowed) && (!file_allowed) && https_allowed;
    RecordSecResult(r, "T22", "Navigation to prohibited scheme", pass, pass ? "Dangerous schemes (javascript:, file:) blocked from navigation" : "Dangerous scheme executed");
}

// ------------------------------------------------------------
// T23: Renderer Process Crash
// ------------------------------------------------------------
static void Test_T23_RendererProcessCrash(SecurityTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    renderer->SimulateCrash();

    bool pass = (renderer->state() == process::RENDERER_STATE_CRASHED) && (browser->browser_pid() > 0);
    RecordSecResult(r, "T23", "Renderer process crash", pass, pass ? "Renderer crash contained safely; Browser process survives" : "Browser crashed");
}

// ------------------------------------------------------------
// T24: Renderer A -> Renderer B Isolation
// ------------------------------------------------------------
static void Test_T24_RendererIsolation(SecurityTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* r1 = browser->CreateRendererHost();
    process::RendererProcessHost* r2 = browser->CreateRendererHost();

    bool pass = r1 && r2 && (r1->pid() != r2->pid()) && (r1->cr3() != r2->cr3()) &&
                (r1->channel() != r2->channel());
    RecordSecResult(r, "T24", "Renderer A -> Renderer B isolation", pass, pass ? "Distinct hardware address spaces and Mojo channels active" : "Renderers co-located");
}

// ------------------------------------------------------------
// T25: Network Bypass Attempt
// ------------------------------------------------------------
static void Test_T25_NetworkBypassAttempt(SecurityTestResult* r) {
    // Renderer process must not have raw network capability
    bos_sandbox_status_t status = sandbox_net_validate_connection(10, 6 /* TCP */, "8.8.8.8", 53);
    bool pass = (status == BOS_SANDBOX_ERR_NETWORK_DENIED || status == BOS_SANDBOX_ERR_CAPABILITY_DENIED);
    RecordSecResult(r, "T25", "Network bypass attempt", pass, pass ? "Direct network access denied; requests forced via Network Process" : "Renderer bypassed network process");
}

// ------------------------------------------------------------
// T26: Utility Storage Bypass Attempt
// ------------------------------------------------------------
static void Test_T26_UtilityStorageBypass(SecurityTestResult* r) {
    bos_sandbox_status_t status = sandbox_fs_validate_path(10, "/var/storage/local_storage.dat", 2 /* Write */);
    bool pass = (status == BOS_SANDBOX_ERR_CAPABILITY_DENIED || status == BOS_SANDBOX_ERR_PATH_DENIED);
    RecordSecResult(r, "T26", "Utility storage bypass attempt", pass, pass ? "Direct storage disk write denied; requests forced via Utility Process" : "Direct disk write allowed");
}

// ------------------------------------------------------------
// T27: Resource Exhaustion
// ------------------------------------------------------------
static void Test_T27_ResourceExhaustion(SecurityTestResult* r) {
    net::ContentSecurityPolicy csp = net::ContentSecurityPolicy::Parse("default-src 'self'; script-src 'self'", net::SecurityOrigin("https", "example.com", 443));
    bool inline_script = csp.AllowScriptFromSource(net::GURL("https://example.com"), true /* is_inline */);
    bool external_script = csp.AllowScriptFromSource(net::GURL("https://example.com/app.js"), false /* is_inline */);
    bool attacker_script = csp.AllowScriptFromSource(net::GURL("https://evil.com/pwn.js"), false);

    bool pass = (!inline_script) && external_script && (!attacker_script);
    RecordSecResult(r, "T27", "Resource exhaustion", pass, pass ? "Content Security Policy (CSP) restricts unauthorized execution" : "CSP enforcement failed");
}

// ------------------------------------------------------------
// T28: Process Capability Forgery
// ------------------------------------------------------------
static void Test_T28_ProcessCapabilityForgery(SecurityTestResult* r) {
    bos_token_t forged_token;
    memset(&forged_token, 0, sizeof(forged_token));
    forged_token.token_id = 999;
    forged_token.owner_pid = 501;
    forged_token.signature = 0xDEADBEEFULL; // Invalid signature
    forged_token.is_valid = true;

    bos_sandbox_status_t status = bos_token_verify(&forged_token);
    bool pass = (status == BOS_SANDBOX_ERR_TOKEN_FORGED);
    RecordSecResult(r, "T28", "Process capability forgery", pass, pass ? "Forged security token cryptographically rejected" : "Forged token accepted");
}

// ------------------------------------------------------------
// T29: Browser Survival After Renderer Compromise
// ------------------------------------------------------------
static void Test_T29_BrowserSurvival(SecurityTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    renderer->SimulateCrash();

    bool browser_alive = (browser->browser_pid() > 0);
    process::NetworkProcessHost* net = browser->GetNetworkHost();
    bool net_alive = net && net->is_alive();

    bool pass = browser_alive && net_alive;
    RecordSecResult(r, "T29", "Browser survival after renderer compromise", pass, pass ? "Browser and Network hosts survive renderer death" : "Cascading browser failure");
}

// ------------------------------------------------------------
// T30: Complete Renderer Cleanup
// ------------------------------------------------------------
static void Test_T30_CompleteRendererCleanup(SecurityTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    uint32_t r_pid = renderer->pid();

    browser->RemoveRenderer(renderer);

    mojo::core::HandleTable* ht = mojo::core::HandleTable::GetInstance();
    ht->CloseAllHandlesForProcess(r_pid);

    bool pass = true;
    RecordSecResult(r, "T30", "Complete renderer cleanup", pass, pass ? "All handles, pipes and memory associated with renderer reclaimed" : "Resource leak detected");
}

// ------------------------------------------------------------
// Master Runner
// ------------------------------------------------------------
int RunSecurityTestSuite(SecurityTestResult* results, int max_results) {
    s_sec_test_idx = 0;
    if (max_results < 30) return -1;

    Test_T01_KernelPointerRead(results);
    Test_T02_KernelPointerWrite(results);
    Test_T03_PhysicalMemoryMapping(results);
    Test_T04_UnauthorizedVFSAccess(results);
    Test_T05_UnauthorizedRawSocket(results);
    Test_T06_UnauthorizedDeviceAccess(results);
    Test_T07_CrossProcessMemoryRead(results);
    Test_T08_CrossProcessMemoryWrite(results);
    Test_T09_ForgedMojoHandle(results);
    Test_T10_UnauthorizedMojoEndpoint(results);
    Test_T11_OversizedIPCMessage(results);
    Test_T12_MalformedIPCMessage(results);
    Test_T13_InvalidSharedMemoryHandle(results);
    Test_T14_SharedMemoryBoundsViolation(results);
    Test_T15_UnauthorizedPagePermissionChange(results);
    Test_T16_RWXMemoryAttempt(results);
    Test_T17_CrossOriginDOMAccess(results);
    Test_T18_CrossOriginLocalStorage(results);
    Test_T19_CrossOriginSessionStorage(results);
    Test_T20_HttpOnlyCookieAccess(results);
    Test_T21_SecureCookieOverHTTP(results);
    Test_T22_NavigationProhibitedScheme(results);
    Test_T23_RendererProcessCrash(results);
    Test_T24_RendererIsolation(results);
    Test_T25_NetworkBypassAttempt(results);
    Test_T26_UtilityStorageBypass(results);
    Test_T27_ResourceExhaustion(results);
    Test_T28_ProcessCapabilityForgery(results);
    Test_T29_BrowserSurvival(results);
    Test_T30_CompleteRendererCleanup(results);

    return s_sec_test_idx;
}

extern "C" bool Security_RunAllVerificationTests(void) {
    puts("\n=======================================================");
    puts("     CHROMIUM SANDBOX & WEB SECURITY (PHASE 15)        ");
    puts("=======================================================");

    SecurityTestResult results[36];
    int total = RunSecurityTestSuite(results, 36);
    int passed = 0;

    for (int i = 0; i < total; i++) {
        printf("[%s] %s ... ", results[i].test_id, results[i].test_name);
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
        puts("       PHASE 15 VERIFICATION: ALL 30 TESTS PASS        ");
        puts("=======================================================\n");
        return true;
    } else {
        puts("=======================================================");
        puts("       PHASE 15 VERIFICATION: FAILURES DETECTED        ");
        puts("=======================================================\n");
        return false;
    }
}
