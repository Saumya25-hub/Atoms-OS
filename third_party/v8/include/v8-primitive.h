/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_PRIMITIVE_H_
#define INCLUDE_V8_PRIMITIVE_H_

#include "v8-value.h"

namespace v8 {

class Primitive : public Value {};

class String : public Primitive {
public:
    enum NewStringType {
        kNormal,
        kInternalized
    };

    class Utf8Value {
    public:
        Utf8Value(Isolate* isolate, Local<Value> obj);
        ~Utf8Value();
        char* operator*() { return str_; }
        const char* operator*() const { return str_; }
        int length() const { return length_; }
    private:
        char* str_;
        int length_;
    };

    static Local<String> NewFromUtf8(Isolate* isolate, const char* data,
                                     NewStringType type = kNormal, int length = -1);
    static Local<String> Empty(Isolate* isolate);
    int Length() const;
    int Utf8Length(Isolate* isolate) const;
    int WriteUtf8(Isolate* isolate, char* buffer, int length = -1) const;

    bool IsString() const override { return true; }
};

class Number : public Primitive {
public:
    static Local<Number> New(Isolate* isolate, double value);
    double Value() const;
    bool IsNumber() const override { return true; }
};

class Integer : public Number {
public:
    static Local<Integer> New(Isolate* isolate, int32_t value);
    static Local<Integer> NewFromUnsigned(Isolate* isolate, uint32_t value);
    int64_t Value() const;
    bool IsInt32() const override { return true; }
};

class Boolean : public Primitive {
public:
    static Local<Boolean> New(Isolate* isolate, bool value);
    bool Value() const;
    bool IsBoolean() const override { return true; }
};

class Undefined : public Primitive {
public:
    static Local<Undefined> New(Isolate* isolate);
    bool IsUndefined() const override { return true; }
};

class Null : public Primitive {
public:
    static Local<Null> New(Isolate* isolate);
    bool IsNull() const override { return true; }
};

} // namespace v8

#endif // INCLUDE_V8_PRIMITIVE_H_
