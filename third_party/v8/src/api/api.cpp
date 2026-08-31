/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "api.h"
#include "third_party/v8/src/interpreter/compiler.h"
#include "userspace/runtime/c/include/stdlib.h"
#include "userspace/runtime/c/include/string.h"
#include "userspace/runtime/c/include/stdio.h"

namespace v8 {

static Platform* g_platform = nullptr;
static EntropySource g_entropy_source = nullptr;

void V8::InitializePlatform(Platform* platform) {
    g_platform = platform;
}

void V8::Initialize() {
    // Initialized
}

bool V8::Dispose() {
    return true;
}

void V8::DisposePlatform() {
    g_platform = nullptr;
}

Platform* V8::GetCurrentPlatform() {
    return g_platform;
}

void V8::SetEntropySource(EntropySource entropy_source) {
    g_entropy_source = entropy_source;
}

const char* V8::GetVersion() {
    return "12.4.254.14 (ATOMS OS x86_64)";
}

// -----------------------------------------------------------------------------
// Isolate Implementation
// -----------------------------------------------------------------------------

static Isolate* g_current_isolate = nullptr;

IsolateImpl::IsolateImpl(Platform* platform)
    : platform_(platform)
    , heap_(platform ? platform->GetPageAllocator() : nullptr)
    , interpreter_(&heap_)
{
}

IsolateImpl::~IsolateImpl() {}

Isolate* Isolate::New(const CreateParams& params) {
    (void)params;
    return new IsolateImpl(g_platform);
}

Isolate* Isolate::GetCurrent() {
    return g_current_isolate;
}

void Isolate::Enter() {
    g_current_isolate = this;
}

void Isolate::Exit() {
    if (g_current_isolate == this) {
        g_current_isolate = nullptr;
    }
}

void Isolate::Dispose() {
    delete static_cast<IsolateImpl*>(this);
}

void Isolate::LowMemoryNotification() {
    static_cast<IsolateImpl*>(this)->heap()->CollectGarbage(true);
}

void Isolate::RequestGarbageCollectionForTesting() {
    static_cast<IsolateImpl*>(this)->heap()->CollectGarbage(true);
}

void* Isolate::GetInternalHeap() {
    return static_cast<IsolateImpl*>(this)->heap();
}

// -----------------------------------------------------------------------------
// Scope Implementations
// -----------------------------------------------------------------------------

Isolate::Scope::Scope(Isolate* isolate) : isolate_(isolate) {
    if (isolate_) isolate_->Enter();
}

Isolate::Scope::~Scope() {
    if (isolate_) isolate_->Exit();
}

HandleScope::HandleScope(Isolate* isolate) : isolate_(isolate) {
    if (isolate_) {
        // Hook into internal root set
        static_cast<IsolateImpl*>(isolate_)->heap()->PushRoot(nullptr);
    }
}

HandleScope::~HandleScope() {
    if (isolate_) {
        static_cast<IsolateImpl*>(isolate_)->heap()->PopRoot();
    }
}

int HandleScope::NumberOfHandles(Isolate* isolate) {
    if (!isolate) return 0;
    return (int)static_cast<IsolateImpl*>(isolate)->heap()->GetRootCount();
}

// -----------------------------------------------------------------------------
// Context Implementation
// -----------------------------------------------------------------------------

class ContextImpl : public Context {
public:
    ContextImpl(Isolate* isolate) : isolate_(isolate), global_obj_(nullptr) {
        IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
        global_obj_ = impl->heap()->AllocateObject();
    }

    Isolate* isolate() { return isolate_; }
    internal::JSObject* global() { return global_obj_; }

private:
    Isolate* isolate_;
    internal::JSObject* global_obj_;
};

static Local<Context> g_current_context;

Local<Context> Context::New(Isolate* isolate, void* extension, Local<Object> global_template) {
    (void)extension; (void)global_template;
    return Local<Context>(new ContextImpl(isolate));
}

Local<Object> Context::Global() {
    ContextImpl* c = static_cast<ContextImpl*>(this);
    return Local<Object>(reinterpret_cast<Object*>(c->global()));
}

Isolate* Context::GetIsolate() {
    return static_cast<ContextImpl*>(this)->isolate();
}

void Context::Enter() {
    g_current_context = Local<Context>(this);
}

void Context::Exit() {
    g_current_context = Local<Context>();
}

Context::Scope::Scope(Local<Context> context) : context_(context) {
    if (!context_.IsEmpty()) context_->Enter();
}

Context::Scope::~Scope() {
    if (!context_.IsEmpty()) context_->Exit();
}

// -----------------------------------------------------------------------------
// Value & String / Number / Boolean Implementations
// -----------------------------------------------------------------------------

Local<String> Value::ToString(Local<Context> context) const {
    Isolate* iso = context->GetIsolate();
    if (IsString()) {
        return Local<String>(const_cast<String*>(static_cast<const String*>(this)));
    }
    if (IsNumber()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%g", NumberValue(context));
        return String::NewFromUtf8(iso, buf);
    }
    return String::NewFromUtf8(iso, "[object Value]");
}

Local<String> String::NewFromUtf8(Isolate* isolate, const char* data, NewStringType type, int length) {
    (void)type;
    if (!isolate || !data) return Local<String>();
    size_t len = (length >= 0) ? (size_t)length : strlen(data);
    IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
    internal::JSString* str = impl->heap()->AllocateString(std::string(data, len));
    return Local<String>(reinterpret_cast<String*>(str));
}

Local<String> String::Empty(Isolate* isolate) {
    return NewFromUtf8(isolate, "");
}

int String::Length() const {
    const internal::JSString* str = reinterpret_cast<const internal::JSString*>(this);
    return (int)str->value.size();
}

int String::Utf8Length(Isolate* isolate) const {
    (void)isolate;
    return Length();
}

int String::WriteUtf8(Isolate* isolate, char* buffer, int length) const {
    (void)isolate;
    const internal::JSString* str = reinterpret_cast<const internal::JSString*>(this);
    size_t copy_len = str->value.size();
    if (length >= 0 && (size_t)length < copy_len) copy_len = (size_t)length;
    memcpy(buffer, str->value.data(), copy_len);
    buffer[copy_len] = '\0';
    return (int)copy_len;
}

String::Utf8Value::Utf8Value(Isolate* isolate, Local<Value> obj) : str_(nullptr), length_(0) {
    if (obj.IsEmpty()) {
        str_ = (char*)malloc(1);
        str_[0] = '\0';
        length_ = 0;
        return;
    }
    if (obj->IsString()) {
        Local<String> s = Local<String>::Cast(obj);
        length_ = s->Length();
        str_ = (char*)malloc(length_ + 1);
        s->WriteUtf8(isolate, str_, length_);
    } else if (obj->IsNumber()) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%g", obj->NumberValue(Local<Context>()));
        length_ = (int)strlen(buf);
        str_ = (char*)malloc(length_ + 1);
        memcpy(str_, buf, length_ + 1);
    } else {
        str_ = (char*)malloc(1);
        str_[0] = '\0';
        length_ = 0;
    }
}


String::Utf8Value::~Utf8Value() {
    if (str_) free(str_);
}

Local<Number> Number::New(Isolate* isolate, double value) {
    IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
    internal::JSNumber* num = impl->heap()->AllocateNumber(value);
    return Local<Number>(reinterpret_cast<Number*>(num));
}

double Number::Value() const {
    const internal::JSNumber* num = reinterpret_cast<const internal::JSNumber*>(this);
    return num->value;
}

Local<Integer> Integer::New(Isolate* isolate, int32_t value) {
    return Local<Integer>::Cast(Number::New(isolate, (double)value));
}

Local<Integer> Integer::NewFromUnsigned(Isolate* isolate, uint32_t value) {
    return Local<Integer>::Cast(Number::New(isolate, (double)value));
}

int64_t Integer::Value() const {
    return (int64_t)Number::Value();
}

Local<Boolean> Boolean::New(Isolate* isolate, bool value) {
    IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
    internal::JSOldBall* b = value ? impl->heap()->GetTrue() : impl->heap()->GetFalse();
    return Local<Boolean>(reinterpret_cast<Boolean*>(b));
}

bool Boolean::Value() const {
    const internal::JSOldBall* b = reinterpret_cast<const internal::JSOldBall*>(this);
    return b->kind == internal::JSOldBall::TRUE_KIND;
}

// -----------------------------------------------------------------------------
// Script Implementation
// -----------------------------------------------------------------------------

class ScriptImpl : public Script {
public:
    ScriptImpl(Isolate* isolate, internal::BytecodeArray* bytecode)
        : isolate_(isolate), bytecode_(bytecode) {}

    Isolate* isolate_;
    internal::BytecodeArray* bytecode_;
};

Local<Script> Script::Compile(Local<Context> context, Local<String> source, ScriptOrigin* origin) {
    (void)origin;
    if (context.IsEmpty() || source.IsEmpty()) return Local<Script>();
    Isolate* iso = context->GetIsolate();
    IsolateImpl* impl = static_cast<IsolateImpl*>(iso);

    String::Utf8Value utf8(iso, source);
    internal::BytecodeArray* bc = internal::Compiler::CompileScript(impl->heap(), *utf8);
    return Local<Script>(new ScriptImpl(iso, bc));
}

Local<Value> Script::Run(Local<Context> context) {
    ScriptImpl* s = static_cast<ScriptImpl*>(this);
    IsolateImpl* impl = static_cast<IsolateImpl*>(s->isolate_);
    internal::HeapObject* res = impl->interpreter()->Execute(s->bytecode_);
    return Local<Value>(reinterpret_cast<Value*>(res));
}

// -----------------------------------------------------------------------------
// Object & Array & Function Implementation
// -----------------------------------------------------------------------------

Local<Object> Object::New(Isolate* isolate) {
    IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
    internal::JSObject* obj = impl->heap()->AllocateObject();
    return Local<Object>(reinterpret_cast<Object*>(obj));
}

bool Object::Set(Local<Context> context, Local<Value> key, Local<Value> value) {
    (void)context;
    internal::JSObject* obj = reinterpret_cast<internal::JSObject*>(this);
    String::Utf8Value k(context->GetIsolate(), key);
    internal::HeapObject* v = reinterpret_cast<internal::HeapObject*>(*value);
    obj->SetProperty(*k, v);
    return true;
}

Local<Value> Object::Get(Local<Context> context, Local<Value> key) {
    (void)context;
    internal::JSObject* obj = reinterpret_cast<internal::JSObject*>(this);
    String::Utf8Value k(context->GetIsolate(), key);
    internal::HeapObject* v = obj->GetProperty(*k);
    return Local<Value>(reinterpret_cast<Value*>(v));
}

Local<Array> Array::New(Isolate* isolate, int length) {
    IsolateImpl* impl = static_cast<IsolateImpl*>(isolate);
    internal::JSArray* arr = impl->heap()->AllocateArray((size_t)(length > 0 ? length : 0));
    return Local<Array>(reinterpret_cast<Array*>(arr));
}

uint32_t Array::Length() const {
    const internal::JSArray* arr = reinterpret_cast<const internal::JSArray*>(this);
    return (uint32_t)arr->Length();
}

} // namespace v8
