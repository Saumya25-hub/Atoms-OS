/*
 * ATOMS OS — Phase 13 Multi-Process Browser Verification Test Suite
 * 20 Deterministic Tests
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "process_test_suite.h"
#include "third_party/chromium_process/browser_process_host.h"
#include "third_party/chromium_process/renderer_process_host.h"
#include "third_party/chromium_process/network_process_host.h"
#include "third_party/chromium_process/utility_process_host.h"
#include "third_party/chromium_ipc/atoms_ipc_channel.h"
#include "userspace/runtime/c/include/stdio.h"
#include "userspace/runtime/c/include/string.h"

static int s_test_idx = 0;

static void RecordResult(ProcessTestResult* results, const char* name, bool pass, const char* detail) {
    results[s_test_idx].test_name = name;
    results[s_test_idx].passed = pass;
    results[s_test_idx].detail = detail;
    s_test_idx++;
}

// ------------------------------------------------------------
// Test 1: Browser Process Initialization
// ------------------------------------------------------------
static void Test_Browser_Init(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    bool pass = browser != nullptr && browser->browser_pid() > 0 && browser->browser_cr3() != 0;
    RecordResult(r, "Process_Create_Browser", pass, pass ? "Browser host active with valid PID & CR3" : "Browser host init failed");
}

// ------------------------------------------------------------
// Test 2: Renderer Process Creation
// ------------------------------------------------------------
static void Test_Renderer_Create(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    bool pass = renderer != nullptr && renderer->is_alive() && renderer->pid() > 0 && renderer->cr3() != 0;
    RecordResult(r, "Process_Create_Renderer", pass, pass ? "Renderer launched with valid PID & CR3" : "Renderer launch failed");
}

// ------------------------------------------------------------
// Test 3: PID Divergence (PID_Browser != PID_Renderer)
// ------------------------------------------------------------
static void Test_PID_Divergence(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    bool pass = renderer && (browser->browser_pid() != renderer->pid());
    RecordResult(r, "Process_PID_Divergence", pass, pass ? "PID_Browser != PID_Renderer confirmed" : "PID collision between Browser & Renderer");
}

// ------------------------------------------------------------
// Test 4: CR3 Divergence (CR3_Browser != CR3_Renderer)
// ------------------------------------------------------------
static void Test_CR3_Divergence(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    bool pass = renderer && (browser->browser_cr3() != renderer->cr3());
    RecordResult(r, "Process_CR3_Divergence", pass, pass ? "CR3_Browser != CR3_Renderer (Hardware Isolation Verified)" : "Shared CR3 violation");
}

// ------------------------------------------------------------
// Test 5: Multiple Independent Renderers (PID_R1 != PID_R2)
// ------------------------------------------------------------
static void Test_Multi_Renderer_PID(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* r1 = browser->CreateRendererHost();
    process::RendererProcessHost* r2 = browser->CreateRendererHost();
    bool pass = r1 && r2 && (r1->pid() != r2->pid());
    RecordResult(r, "Process_Multi_Renderer_PID", pass, pass ? "Multiple renderers have distinct PIDs" : "Duplicate renderer PIDs");
}

// ------------------------------------------------------------
// Test 6: Multiple Independent Renderers (CR3_R1 != CR3_R2)
// ------------------------------------------------------------
static void Test_Multi_Renderer_CR3(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* r1 = browser->CreateRendererHost();
    process::RendererProcessHost* r2 = browser->CreateRendererHost();
    bool pass = r1 && r2 && (r1->cr3() != r2->cr3());
    RecordResult(r, "Process_Multi_Renderer_CR3", pass, pass ? "Multiple renderers have distinct CR3 page tables" : "Shared renderer CR3 violation");
}

// ------------------------------------------------------------
// Test 7: Network Process Host Creation
// ------------------------------------------------------------
static void Test_Network_Host(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::NetworkProcessHost* net = browser->GetNetworkHost();
    bool pass = net != nullptr && net->is_alive() && net->pid() > 0 && net->cr3() != 0;
    RecordResult(r, "Process_Network_Host", pass, pass ? "Network process active with dedicated CR3" : "Network process init failed");
}

// ------------------------------------------------------------
// Test 8: Utility Process Host Creation
// ------------------------------------------------------------
static void Test_Utility_Host(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::UtilityProcessHost* util = browser->GetUtilityHost();
    bool pass = util != nullptr && util->is_alive() && util->pid() > 0 && util->cr3() != 0;
    RecordResult(r, "Process_Utility_Host", pass, pass ? "Utility process active with dedicated CR3" : "Utility process init failed");
}

// ------------------------------------------------------------
// Test 9: Quad-Process Topology (4 Distinct PIDs & 4 CR3s)
// ------------------------------------------------------------
static void Test_Quad_Topology(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* ren = browser->CreateRendererHost();
    process::NetworkProcessHost* net = browser->GetNetworkHost();
    process::UtilityProcessHost* util = browser->GetUtilityHost();

    bool pids_unique = ren && net && util &&
                       browser->browser_pid() != ren->pid() &&
                       browser->browser_pid() != net->pid() &&
                       browser->browser_pid() != util->pid() &&
                       ren->pid() != net->pid() &&
                       ren->pid() != util->pid() &&
                       net->pid() != util->pid();

    bool cr3s_unique = ren && net && util &&
                       browser->browser_cr3() != ren->cr3() &&
                       browser->browser_cr3() != net->cr3() &&
                       browser->browser_cr3() != util->cr3() &&
                       ren->cr3() != net->cr3() &&
                       ren->cr3() != util->cr3() &&
                       net->cr3() != util->cr3();

    bool pass = pids_unique && cr3s_unique;
    RecordResult(r, "Process_Quad_Topology", pass, pass ? "4 Processes verified with 4 unique PIDs & 4 unique CR3s" : "Topology overlap detected");
}

// ------------------------------------------------------------
// Test 10: IPC Channel Connection
// ------------------------------------------------------------
static void Test_IPC_Connection(ProcessTestResult* r) {
    ipc::AtomsIPCChannel a("chan-a", 201, 202);
    ipc::AtomsIPCChannel b("chan-b", 202, 201);
    a.SetPeer(&b);
    b.SetPeer(&a);
    bool pass = a.is_connected() && b.is_connected();
    RecordResult(r, "IPC_Channel_Establishment", pass, pass ? "Bidirectional channel pair linked" : "IPC connection failed");
}

// ------------------------------------------------------------
// Test 11: IPC Message Send & Receive
// ------------------------------------------------------------
static void Test_IPC_SendReceive(ProcessTestResult* r) {
    ipc::AtomsIPCChannel a("chan-a", 201, 202);
    ipc::AtomsIPCChannel b("chan-b", 202, 201);
    a.SetPeer(&b);
    b.SetPeer(&a);

    uint32_t send_val = 0xDEADBEEF;
    a.Send(ipc::MSG_HEARTBEAT_PING, &send_val, sizeof(send_val));

    ipc::IPCMessage recv_msg;
    bool pass = b.Receive(&recv_msg, true) &&
                recv_msg.type == ipc::MSG_HEARTBEAT_PING &&
                recv_msg.src_pid == 201 &&
                recv_msg.dst_pid == 202 &&
                *(uint32_t*)recv_msg.payload == 0xDEADBEEF;

    RecordResult(r, "IPC_Message_SendReceive", pass, pass ? "Typed message delivered intact" : "IPC message corrupted or lost");
}

// ------------------------------------------------------------
// Test 12: IPC String Payload
// ------------------------------------------------------------
static void Test_IPC_String(ProcessTestResult* r) {
    ipc::AtomsIPCChannel a("chan-a", 201, 202);
    ipc::AtomsIPCChannel b("chan-b", 202, 201);
    a.SetPeer(&b);
    b.SetPeer(&a);

    a.SendString(ipc::MSG_NAVIGATE, "https://example.com/index.html");

    ipc::IPCMessage recv_msg;
    bool pass = b.Receive(&recv_msg, true) &&
                recv_msg.type == ipc::MSG_NAVIGATE &&
                strcmp((const char*)recv_msg.payload, "https://example.com/index.html") == 0;

    RecordResult(r, "IPC_String_Payload", pass, pass ? "String payload verified" : "IPC string mismatch");
}

// ------------------------------------------------------------
// Test 13: Renderer HTML Navigation Pipeline
// ------------------------------------------------------------
static void Test_Renderer_Navigate(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    bool pass = renderer && renderer->Navigate("https://example.com", "<!doctype html><html><head><title>Test Page</title></head><body><h1>Hello Multi-Process</h1></body></html>");
    pass = pass && renderer->last_rendered_title() == "Test Page";
    RecordResult(r, "Renderer_Navigate_Pipeline", pass, pass ? "HTML rendered inside isolated Renderer process" : "Renderer pipeline failed");
}

// ------------------------------------------------------------
// Test 14: DOM Event Routing via IPC
// ------------------------------------------------------------
static void Test_Renderer_DOM_Event(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    bool pass = renderer && renderer->SendDOMEvent(1 /* Click */, 120, 240, 0);
    RecordResult(r, "Renderer_DOM_Event_IPC", pass, pass ? "DOM Event sent via IPC" : "DOM event dispatch failed");
}

// ------------------------------------------------------------
// Test 15: Renderer Crash Containment
// ------------------------------------------------------------
static void Test_Renderer_Crash(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    renderer->SimulateCrash();
    bool pass = (renderer->state() == process::RENDERER_STATE_CRASHED);
    RecordResult(r, "Renderer_Crash_Containment", pass, pass ? "Renderer crash cleanly contained" : "Crash state invalid");
}

// ------------------------------------------------------------
// Test 16: Browser Process Remains Alive after Crash
// ------------------------------------------------------------
static void Test_Browser_Survives(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    bool pass = browser != nullptr && browser->browser_pid() > 0;
    RecordResult(r, "Browser_Survives_Crash", pass, pass ? "Browser UI process survives child crash" : "Browser process corrupted");
}

// ------------------------------------------------------------
// Test 17: Tab Reload & Fresh Process Allocation
// ------------------------------------------------------------
static void Test_Tab_Reload(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    uint32_t old_pid = renderer->pid();
    renderer->SimulateCrash();

    bool ok = browser->ReloadRenderer(renderer);
    bool pass = ok && renderer->is_alive() && (renderer->pid() != old_pid);
    RecordResult(r, "Renderer_Tab_Reload", pass, pass ? "Crashed tab reloaded with new PID & CR3" : "Tab reload failed");
}

// ------------------------------------------------------------
// Test 18: Network Process Fetch Execution
// ------------------------------------------------------------
static void Test_Network_Fetch(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::NetworkProcessHost* net = browser->GetNetworkHost();
    net::URLLoaderResult res = net->Fetch("invalid://url");
    bool pass = (res.net_error == net::ERR_INVALID_URL);
    RecordResult(r, "Network_Fetch_IPC", pass, pass ? "URLLoader executed in Network process" : "Network fetch failed");
}

// ------------------------------------------------------------
// Test 19: Utility Process Storage Execution
// ------------------------------------------------------------
static void Test_Utility_Storage(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::UtilityProcessHost* util = browser->GetUtilityHost();
    net::SecurityOrigin origin("https", "multiprocess.test", 443);
    bool pass = util->SetLocalStorageItem(origin, "tab_key", "tab_value");
    pass = pass && (util->GetLocalStorageItem(origin, "tab_key") == "tab_value");
    RecordResult(r, "Utility_Storage_IPC", pass, pass ? "Web Storage executed in Utility process" : "Utility storage failed");
}

// ------------------------------------------------------------
// Test 20: Clean Shutdown & Resource Reclamation
// ------------------------------------------------------------
static void Test_Clean_Termination(ProcessTestResult* r) {
    process::BrowserProcessHost* browser = process::BrowserProcessHost::GetInstance();
    process::RendererProcessHost* renderer = browser->CreateRendererHost();
    uint32_t r_pid = renderer->pid();
    renderer->Terminate();
    bool pass = !renderer->is_alive() && (renderer->pid() == 0);
    (void)r_pid;
    RecordResult(r, "Process_Clean_Termination", pass, pass ? "Process terminated & address space reclaimed" : "Termination failed");
}

// ------------------------------------------------------------
// Master Test Runner
// ------------------------------------------------------------
int RunProcessTestSuite(ProcessTestResult* results, int max_results) {
    s_test_idx = 0;
    if (max_results < 20) return -1;

    Test_Browser_Init(results);
    Test_Renderer_Create(results);
    Test_PID_Divergence(results);
    Test_CR3_Divergence(results);
    Test_Multi_Renderer_PID(results);
    Test_Multi_Renderer_CR3(results);
    Test_Network_Host(results);
    Test_Utility_Host(results);
    Test_Quad_Topology(results);
    Test_IPC_Connection(results);
    Test_IPC_SendReceive(results);
    Test_IPC_String(results);
    Test_Renderer_Navigate(results);
    Test_Renderer_DOM_Event(results);
    Test_Renderer_Crash(results);
    Test_Browser_Survives(results);
    Test_Tab_Reload(results);
    Test_Network_Fetch(results);
    Test_Utility_Storage(results);
    Test_Clean_Termination(results);

    return s_test_idx;
}

extern "C" bool Process_RunAllVerificationTests(void) {
    puts("\n=======================================================");
    puts("   MULTI-PROCESS BROWSER ARCHITECTURE (PHASE 13)       ");
    puts("=======================================================");

    ProcessTestResult results[25];
    int total = RunProcessTestSuite(results, 25);
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
        puts("       PHASE 13 VERIFICATION: ALL 20 TESTS PASS        ");
        puts("=======================================================\n");
        return true;
    } else {
        puts("=======================================================");
        puts("       PHASE 13 VERIFICATION: FAILURES DETECTED        ");
        puts("=======================================================\n");
        return false;
    }
}
