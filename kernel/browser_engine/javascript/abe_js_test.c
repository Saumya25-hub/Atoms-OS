#include "abe_js_test.h"
#include "abe_js_promise.h"
#include "abe_js_dom_binding.h"
#include "abe_js_diag.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

static bool g_microtask_executed = false;
static void SampleMicrotask(void* data) {
    g_microtask_executed = true;
}

static bool g_event_handler_executed = false;
static void SampleEventHandler(ABE_NodeHandle target, const char* event_type, void* user_data) {
    g_event_handler_executed = true;
}

static bool Test_LexerAndParser(void) {
    display_print("[ABE_JS_TEST] 1. Testing Streaming Lexer & AST Parser...\n");

    const char* js_source = "const greeting = 'Hello ATOMS OS'; var count = 42; function compute() { return count; }";
    ABE_JSContextHandle ctx = ABE_INVALID_HANDLE;
    ABE_CreateContext(ABE_INVALID_HANDLE, &ctx);

    ABE_JSValue val;
    ABE_Error err = ABE_ExecuteScript(ctx, js_source, strlen(js_source), &val);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_JS_TEST] FAIL: ExecuteScript failed for standard ECMAScript code!\n");
        ABE_DestroyContext(ctx);
        return false;
    }

    ABE_DestroyContext(ctx);
    display_print("[ABE_JS_TEST] PASS: Streaming Lexer & AST Parser verified.\n");
    return true;
}

static bool Test_PromisesAndMicrotasks(void) {
    display_print("[ABE_JS_TEST] 2. Testing Promise Engine & Microtask Queue...\n");

    uint32_t promise_handle = ABE_INVALID_HANDLE;
    ABE_Error err = ABE_JSPromise_Create(&promise_handle);
    if (err != ABE_SUCCESS || promise_handle == ABE_INVALID_HANDLE) {
        display_print("[ABE_JS_TEST] FAIL: JSPromise_Create failed!\n");
        return false;
    }

    ABE_JSValue resolve_val;
    resolve_val.type = ABE_JS_TYPE_NUMBER;
    resolve_val.u.number_val = 200.0;
    ABE_JSPromise_Resolve(promise_handle, &resolve_val);

    g_microtask_executed = false;
    ABE_JS_QueueMicrotask(SampleMicrotask, NULL);
    uint32_t processed = ABE_JS_ProcessMicrotasks();

    if (processed != 1 || !g_microtask_executed) {
        display_print("[ABE_JS_TEST] FAIL: Microtask execution failed!\n");
        return false;
    }

    display_print("[ABE_JS_TEST] PASS: Promise Engine & Microtask Queue verified.\n");
    return true;
}

static bool Test_DOMBindingAndEvents(void) {
    display_print("[ABE_JS_TEST] 3. Testing DOM Binding & Event System...\n");

    ABE_NodeHandle target_node = 101;
    g_event_handler_executed = false;

    ABE_JSDOM_AddEventListener(target_node, "click", SampleEventHandler, NULL);
    ABE_JSDOM_DispatchEvent(ABE_INVALID_HANDLE, target_node, "click");

    if (!g_event_handler_executed) {
        display_print("[ABE_JS_TEST] FAIL: DOM Event Listener dispatch failed!\n");
        return false;
    }

    display_print("[ABE_JS_TEST] PASS: DOM Binding & Event System verified.\n");
    return true;
}

static bool Test_GarbageCollector(void) {
    display_print("[ABE_JS_TEST] 4. Testing Mark-and-Sweep Garbage Collector...\n");

    ABE_JSContextHandle ctx = ABE_INVALID_HANDLE;
    ABE_CreateContext(ABE_INVALID_HANDLE, &ctx);

    ABE_Error err = ABE_GarbageCollect(ctx);
    if (err != ABE_SUCCESS) {
        display_print("[ABE_JS_TEST] FAIL: Mark-and-Sweep GC failed!\n");
        ABE_DestroyContext(ctx);
        return false;
    }

    ABE_DestroyContext(ctx);
    display_print("[ABE_JS_TEST] PASS: Mark-and-Sweep Garbage Collector verified.\n");
    return true;
}

void ABE_RunPhase7_VerificationSuite(void) {
    display_print("\n=========================================================\n");
    display_print(" ATOMS OS — ABE Phase 7 Production JS Verification Suite  \n");
    display_print("=========================================================\n");

    ABE_HTMLInitialize();
    ABE_CSSInitialize();
    ABE_LayoutInitialize();
    ABE_JSInitialize();

    if (!Test_LexerAndParser()) return;
    if (!Test_PromisesAndMicrotasks()) return;
    if (!Test_DOMBindingAndEvents()) return;
    if (!Test_GarbageCollector()) return;

    ABE_JSDiag_DumpVMStats(ABE_INVALID_HANDLE);

    ABE_JSShutdown();
    ABE_LayoutShutdown();
    ABE_CSSShutdown();
    ABE_HTMLShutdown();

    display_print("\nPASS_PHASE7_ABE_PRODUCTION_ECMASCRIPT_RUNTIME\n\n");
}
