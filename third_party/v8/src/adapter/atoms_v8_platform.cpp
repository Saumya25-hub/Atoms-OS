/*
 * ATOMS OS — Google V8 Engine Platform Adapter Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "atoms_v8_platform.h"
#include "third_party/v8/include/v8.h"
#include "third_party/v8/src/base/platform/platform.h"
#include "userspace/runtime/c/include/stdio.h"
#include "userspace/runtime/c/include/string.h"

namespace {
v8::base::AtomsDefaultPlatform* g_atoms_platform = nullptr;
}

extern "C" {

bool AtomsV8_Initialize(void) {
    if (!g_atoms_platform) {
        g_atoms_platform = new v8::base::AtomsDefaultPlatform();
        v8::V8::InitializePlatform(g_atoms_platform);
        v8::V8::Initialize();
    }
    return true;
}

void AtomsV8_Shutdown(void) {
    if (g_atoms_platform) {
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        delete g_atoms_platform;
        g_atoms_platform = nullptr;
    }
}

bool AtomsV8_ExecuteScript(const char* js_source, char* out_buf, size_t out_len) {
    if (!js_source) return false;
    AtomsV8_Initialize();

    v8::Isolate::CreateParams create_params;
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    if (!isolate) return false;

    bool success = false;
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);

        v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, js_source);
        v8::Local<v8::Script> script = v8::Script::Compile(context, source);
        if (!script.IsEmpty()) {
            v8::Local<v8::Value> result = script->Run(context);
            if (!result.IsEmpty()) {
                v8::String::Utf8Value utf8(isolate, result);
                if (out_buf && out_len > 0) {
                    snprintf(out_buf, out_len, "%s", *utf8);
                }
                success = true;
            }
        }
    }

    isolate->Dispose();
    return success;
}

} // extern "C"
