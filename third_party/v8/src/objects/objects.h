/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_OBJECTS_OBJECTS_H_
#define V8_OBJECTS_OBJECTS_H_

#include "third_party/v8/include/v8.h"
#include "userspace/runtime/cpp/include/string"
#include "userspace/runtime/cpp/include/vector"

namespace v8 {
namespace internal {

enum InstanceType {
    ODDBALL_TYPE,
    HEAP_NUMBER_TYPE,
    STRING_TYPE,
    JS_OBJECT_TYPE,
    JS_ARRAY_TYPE,
    JS_FUNCTION_TYPE,
    CODE_TYPE
};

class Heap;

class HeapObject {
public:
    InstanceType type;
    bool is_marked;
    HeapObject* forward_ptr;

    HeapObject(InstanceType t) : type(t), is_marked(false), forward_ptr(nullptr) {}
    virtual ~HeapObject() = default;
};

class JSString : public HeapObject {
public:
    std::string value;

    JSString(const std::string& str) : HeapObject(STRING_TYPE), value(str) {}
    JSString(const char* data, size_t len) : HeapObject(STRING_TYPE), value(data, len) {}
};

class JSNumber : public HeapObject {
public:
    double value;

    JSNumber(double v) : HeapObject(HEAP_NUMBER_TYPE), value(v) {}
};

class JSOldBall : public HeapObject {
public:
    enum Kind { UNDEFINED_KIND, NULL_KIND, TRUE_KIND, FALSE_KIND };
    Kind kind;

    JSOldBall(Kind k) : HeapObject(ODDBALL_TYPE), kind(k) {}
};

struct Property {
    std::string name;
    HeapObject* value;
};

class JSObject : public HeapObject {
public:
    std::vector<Property> properties;

    JSObject() : HeapObject(JS_OBJECT_TYPE) {}

    void SetProperty(const std::string& name, HeapObject* val);
    HeapObject* GetProperty(const std::string& name);
    bool HasProperty(const std::string& name);
    bool DeleteProperty(const std::string& name);
};

class JSArray : public JSObject {
public:
    std::vector<HeapObject*> elements;

    JSArray(size_t initial_len = 0) {
        type = JS_ARRAY_TYPE;
        elements.resize(initial_len);
    }

    void Push(HeapObject* val) { elements.push_back(val); }
    size_t Length() const { return elements.size(); }
    HeapObject* GetElement(size_t index) {
        return index < elements.size() ? elements[index] : nullptr;
    }
    void SetElement(size_t index, HeapObject* val) {
        if (index >= elements.size()) elements.resize(index + 1);
        elements[index] = val;
    }

};

class BytecodeArray;

typedef HeapObject* (*NativeFunctionPtr)(Isolate* isolate, JSObject* receiver, int argc, HeapObject** argv);

class JSFunction : public JSObject {
public:
    std::string name;
    BytecodeArray* bytecode;
    NativeFunctionPtr native_callback;
    void* jit_entry_point;

    JSFunction(const std::string& n, BytecodeArray* bc = nullptr)
        : name(n), bytecode(bc), native_callback(nullptr), jit_entry_point(nullptr) {
        type = JS_FUNCTION_TYPE;
    }

    JSFunction(const std::string& n, NativeFunctionPtr callback)
        : name(n), bytecode(nullptr), native_callback(callback), jit_entry_point(nullptr) {
        type = JS_FUNCTION_TYPE;
    }
};

} // namespace internal
} // namespace v8

#endif // V8_OBJECTS_OBJECTS_H_
