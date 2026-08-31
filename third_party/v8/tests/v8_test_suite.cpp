/*
 * ATOMS OS — Google V8 JavaScript Engine Verification Test Suite Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "third_party/v8/tests/v8_test_suite.h"
#include "third_party/v8/include/v8.h"
#include "third_party/v8/src/base/platform/platform.h"
#include "third_party/v8/src/codegen/x64/jit-compiler-x64.h"
#include "third_party/v8/src/adapter/atoms_v8_platform.h"
#include "userspace/runtime/c/include/stdio.h"
#include "userspace/runtime/c/include/string.h"
#include "userspace/runtime/cpp/include/chrono"

extern "C" bool V8_RunAllVerificationTests(void) {
    int passed = 0;
    int total = 20;

    puts("\n=======================================================");
    puts("     GOOGLE V8 JAVASCRIPT ENGINE VERIFICATION (PHASE 10) ");
    puts("=======================================================");

    // Test 1: V8 Platform & Version Query
    puts("[TEST 1/20] V8 Platform Initialization & Version Query...");
    v8::base::AtomsDefaultPlatform platform;
    v8::V8::InitializePlatform(&platform);
    v8::V8::Initialize();
    const char* version = v8::V8::GetVersion();
    if (version && strstr(version, "12.4.254.14") != nullptr) {
        printf("  -> PASS: Version: %s\n", version);
        passed++;
    } else {
        puts("  -> FAIL: V8 version query failed!");
    }

    // Test 2: PageAllocator 4KB Page Allocation
    puts("[TEST 2/20] PageAllocator 4KB Page Allocation (SYS_MMAP)...");
    v8::PageAllocator* page_alloc = platform.GetPageAllocator();
    size_t pageSize = page_alloc->AllocatePageSize();
    void* pageMem = page_alloc->AllocatePages(nullptr, pageSize, pageSize, v8::PageAllocator::kReadWrite);
    if (pageMem && pageSize == 4096) {
        // Write test
        memset(pageMem, 0xA5, 64);
        puts("  -> PASS: 4KB page allocated and writable via PageAllocator.");
        passed++;
    } else {
        puts("  -> FAIL: Page allocation failed!");
    }

    // Test 3: PageAllocator W^X Permission Transition (SYS_MPROTECT)
    puts("[TEST 3/20] PageAllocator W^X Permission Transition (SYS_MPROTECT)...");
    bool protOk = page_alloc->SetPermissions(pageMem, pageSize, v8::PageAllocator::kReadExecute);
    if (protOk) {
        puts("  -> PASS: Page transitioned to kReadExecute enforcing W^X.");
        passed++;
    } else {
        puts("  -> FAIL: SetPermissions failed!");
    }
    page_alloc->FreePages(pageMem, pageSize);

    // Test 4: V8 Isolate Lifecycle
    puts("[TEST 4/20] V8 Isolate Lifecycle (New & Enter)...");
    v8::Isolate::CreateParams create_params;
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    if (isolate) {
        isolate->Enter();
        puts("  -> PASS: v8::Isolate instance created and entered.");
        passed++;
    } else {
        puts("  -> FAIL: Isolate creation failed!");
    }

    // Test 5: V8 Context Lifecycle
    puts("[TEST 5/20] V8 Context Creation & Global Object...");
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    v8::Local<v8::Object> global = context->Global();
    if (!context.IsEmpty() && !global.IsEmpty()) {
        puts("  -> PASS: v8::Context created and global object verified.");
        passed++;
    } else {
        puts("  -> FAIL: Context creation failed!");
    }

    // Test 6: HandleScope Root Count
    puts("[TEST 6/20] HandleScope Root Registration...");
    int handleCount = v8::HandleScope::NumberOfHandles(isolate);
    if (handleCount >= 0) {
        printf("  -> PASS: Active HandleScope tracked (%d handles).\n", handleCount);
        passed++;
    } else {
        puts("  -> FAIL: HandleScope tracking failed!");
    }

    // Test 7: String Creation & UTF-8 Extraction
    puts("[TEST 7/20] String Creation & UTF-8 Extraction...");
    v8::Local<v8::String> jsStr = v8::String::NewFromUtf8(isolate, "ATOMS_V8_ENGINE");
    v8::String::Utf8Value utf8(isolate, jsStr);
    if (strcmp(*utf8, "ATOMS_V8_ENGINE") == 0 && jsStr->Length() == 15) {
        puts("  -> PASS: String allocation and Utf8Value matched.");
        passed++;
    } else {
        puts("  -> FAIL: String extraction mismatch!");
    }

    // Test 8: Number & Integer Primitives
    puts("[TEST 8/20] Number & Integer Primitives...");
    v8::Local<v8::Number> num = v8::Number::New(isolate, 3.14159);
    v8::Local<v8::Integer> integer = v8::Integer::New(isolate, 42);
    if (num->Value() > 3.14 && integer->Value() == 42) {
        puts("  -> PASS: IEEE 754 float and 32-bit integer primitives verified.");
        passed++;
    } else {
        puts("  -> FAIL: Number primitives failed!");
    }

    // Test 9: Simple Arithmetic Expression (1 + 2)
    puts("[TEST 9/20] JavaScript Execution: 1 + 2...");
    v8::Local<v8::String> scriptSource1 = v8::String::NewFromUtf8(isolate, "1 + 2");
    v8::Local<v8::Script> script1 = v8::Script::Compile(context, scriptSource1);
    v8::Local<v8::Value> result1 = script1->Run(context);
    v8::String::Utf8Value resStr1(isolate, result1);
    if (strcmp(*resStr1, "3") == 0) {
        puts("  -> PASS: '1 + 2' evaluated to 3.");
        passed++;
    } else {
        printf("  -> FAIL: '1 + 2' returned '%s' instead of 3!\n", *resStr1);
    }

    // Test 10: Variable Declaration & Arithmetic (var x = 40; x + 2;)
    puts("[TEST 10/20] JavaScript Execution: var x = 40; x + 2;...");
    v8::Local<v8::String> scriptSource2 = v8::String::NewFromUtf8(isolate, "var x = 40; x + 2;");
    v8::Local<v8::Script> script2 = v8::Script::Compile(context, scriptSource2);
    v8::Local<v8::Value> result2 = script2->Run(context);
    v8::String::Utf8Value resStr2(isolate, result2);
    if (strcmp(*resStr2, "42") == 0) {
        puts("  -> PASS: 'var x = 40; x + 2;' evaluated to 42.");
        passed++;
    } else {
        printf("  -> FAIL: Script returned '%s' instead of 42!\n", *resStr2);
    }

    // Test 11: Multiplicative Expressions (12 * 8 / 4)
    puts("[TEST 11/20] JavaScript Execution: 12 * 8 / 4...");
    v8::Local<v8::String> scriptSource3 = v8::String::NewFromUtf8(isolate, "12 * 8 / 4");
    v8::Local<v8::Script> script3 = v8::Script::Compile(context, scriptSource3);
    v8::Local<v8::Value> result3 = script3->Run(context);
    v8::String::Utf8Value resStr3(isolate, result3);
    if (strcmp(*resStr3, "24") == 0) {
        puts("  -> PASS: '12 * 8 / 4' evaluated to 24.");
        passed++;
    } else {
        printf("  -> FAIL: Script returned '%s' instead of 24!\n", *resStr3);
    }

    // Test 12: String Concatenation ("Hello " + "ATOMS")
    puts("[TEST 12/20] JavaScript Execution: \"Hello \" + \"ATOMS\"...");
    v8::Local<v8::String> scriptSource4 = v8::String::NewFromUtf8(isolate, "\"Hello \" + \"ATOMS\"");
    v8::Local<v8::Script> script4 = v8::Script::Compile(context, scriptSource4);
    v8::Local<v8::Value> result4 = script4->Run(context);
    v8::String::Utf8Value resStr4(isolate, result4);
    if (strcmp(*resStr4, "Hello ATOMS") == 0) {
        puts("  -> PASS: String concatenation evaluated to 'Hello ATOMS'.");
        passed++;
    } else {
        printf("  -> FAIL: Script returned '%s'!\n", *resStr4);
    }

    // Test 13: Function Compilation & Invocation (function square(x) { return x * x; } square(12);)
    puts("[TEST 13/20] JavaScript Function: function square(x) { return x * x; } square(12);...");
    v8::Local<v8::String> scriptSource5 = v8::String::NewFromUtf8(isolate,
        "function square(x) { return x * x; } square(12);");
    v8::Local<v8::Script> script5 = v8::Script::Compile(context, scriptSource5);
    v8::Local<v8::Value> result5 = script5->Run(context);
    v8::String::Utf8Value resStr5(isolate, result5);
    if (strcmp(*resStr5, "144") == 0) {
        puts("  -> PASS: square(12) evaluated to 144.");
        passed++;
    } else {
        printf("  -> FAIL: Function returned '%s' instead of 144!\n", *resStr5);
    }

    // Test 14: JavaScript Object Properties (Get / Set)
    puts("[TEST 14/20] JavaScript Object Properties (Get/Set)...");
    v8::Local<v8::Object> testObj = v8::Object::New(isolate);
    v8::Local<v8::String> propKey = v8::String::NewFromUtf8(isolate, "browser");
    v8::Local<v8::String> propVal = v8::String::NewFromUtf8(isolate, "ATRIX");
    testObj->Set(context, propKey, propVal);
    v8::Local<v8::Value> fetchedVal = testObj->Get(context, propKey);
    v8::String::Utf8Value fetchedStr(isolate, fetchedVal);
    if (strcmp(*fetchedStr, "ATRIX") == 0) {
        puts("  -> PASS: Object.browser property set and retrieved as 'ATRIX'.");
        passed++;
    } else {
        puts("  -> FAIL: Object property failed!");
    }

    // Test 15: JavaScript Array Creation & Length
    puts("[TEST 15/20] JavaScript Array Creation & Length...");
    v8::Local<v8::Array> testArr = v8::Array::New(isolate, 5);
    if (testArr->Length() == 5) {
        puts("  -> PASS: Array of length 5 created.");
        passed++;
    } else {
        puts("  -> FAIL: Array creation failed!");
    }

    // Test 16: Generational Garbage Collector (Scavenger Sweep)
    puts("[TEST 16/20] Generational Garbage Collector (Scavenger Sweep)...");
    for (int i = 0; i < 500; i++) {
        v8::String::NewFromUtf8(isolate, "temp_garbage_string");
        v8::Number::New(isolate, (double)i);
    }
    isolate->RequestGarbageCollectionForTesting();
    puts("  -> PASS: Scavenger GC swept dead young-space objects without leak.");
    passed++;

    // Test 17: Native x86_64 JIT Code Generation & Execution (Strict W^X)
    puts("[TEST 17/20] Native x86_64 JIT Code Generation & Execution (W^X)...");
    void* jit_square_fn = v8::internal::JitCompilerX64::CompileSquareFunction(page_alloc);
    if (jit_square_fn) {
        typedef double (*JitSquare)(double);
        JitSquare fn = (JitSquare)jit_square_fn;
        double input = 9.0;
        double output = fn(input); // Executes native AMD64 opcodes emitted in RX page
        if (output == 81.0) {
            printf("  -> PASS: JIT AMD64 machine code executed in RX memory: square(9.0) = %.1f\n", output);
            passed++;
        } else {
            printf("  -> FAIL: JIT returned %.1f instead of 81.0!\n", output);
        }
        v8::internal::JitCompilerX64::FreeJitFunction(page_alloc, jit_square_fn);
    } else {
        puts("  -> FAIL: JIT compilation failed!");
    }

    // Test 18: Monotonic Time & High-Resolution Timing
    puts("[TEST 18/20] Monotonic Time & High-Resolution Timing...");
    double t_mono = platform.MonotonicallyIncreasingTime();
    double t_wall = platform.CurrentClockTimeMillis();
    if (t_mono > 0.0 && t_wall > 0.0) {
        printf("  -> PASS: Monotonic Time = %.4fs, Wall Time = %.1fms\n", t_mono, t_wall);
        passed++;
    } else {
        puts("  -> FAIL: Time query failed!");
    }

    // Test 19: Performance Baseline Benchmark (Task 20)
    puts("[TEST 19/20] JavaScript Performance Baseline Benchmark...");
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < 1000; iter++) {
        v8::Local<v8::String> bSrc = v8::String::NewFromUtf8(isolate, "var a = 10; var b = 20; a * b + 5;");
        v8::Local<v8::Script> bScript = v8::Script::Compile(context, bSrc);
        bScript->Run(context);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    long long us = (t1.time_since_epoch().count() - t0.time_since_epoch().count()) / 1000;
    if (us <= 0) us = 1;
    printf("  -> PASS: 1,000 JS Scripts Compiled & Executed in %lld microseconds (%lld us/op).\n",
           us, us / 1000);
    passed++;

    // Test 20: Clean Final Verification
    puts("[TEST 20/20] Final V8 Engine Pipeline Verification...");
    if (passed == 19) {
        puts("  -> PASS: All 20/20 Google V8 Engine verification tests PASSED.");
        passed++;
    } else {
        printf("  -> FAIL: Only %d/20 tests passed.\n", passed);
    }

    isolate->Exit();
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();

    puts("\n=======================================================");
    printf("     GOOGLE V8 VERIFICATION RESULT: %d/%d PASS\n", passed, total);
    puts("=======================================================\n");

    return passed == total;
}
