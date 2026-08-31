/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_VALUE_H_
#define INCLUDE_V8_VALUE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

namespace v8 {

class Isolate;
class Context;
class String;
class Number;
class Boolean;
class Object;
class Array;
class Function;

template <class T>
class Local {
public:
    Local() : val_(nullptr) {}
    Local(T* that) : val_(that) {}

    template <class S>
    Local(Local<S> that) : val_(static_cast<T*>(*that)) {}

    bool IsEmpty() const { return val_ == nullptr; }
    T* operator->() const { return val_; }
    T* operator*() const { return val_; }

    template <class S>
    static Local<T> Cast(Local<S> that) {
        return Local<T>(static_cast<T*>(*that));
    }

private:
    T* val_;
};


class Value {
public:
    virtual ~Value() = default;

    virtual bool IsUndefined() const { return false; }
    virtual bool IsNull() const { return false; }
    virtual bool IsTrue() const { return false; }
    virtual bool IsFalse() const { return false; }
    virtual bool IsBoolean() const { return false; }
    virtual bool IsNumber() const { return false; }
    virtual bool IsInt32() const { return false; }
    virtual bool IsUint32() const { return false; }
    virtual bool IsString() const { return false; }
    virtual bool IsObject() const { return false; }
    virtual bool IsArray() const { return false; }
    virtual bool IsFunction() const { return false; }

    virtual double NumberValue(Local<Context> context) const { (void)context; return 0.0; }
    virtual int64_t IntegerValue(Local<Context> context) const { (void)context; return 0; }
    virtual uint32_t Uint32Value(Local<Context> context) const { (void)context; return 0; }
    virtual int32_t Int32Value(Local<Context> context) const { (void)context; return 0; }
    virtual bool BooleanValue(Isolate* isolate) const { (void)isolate; return false; }
    virtual Local<String> ToString(Local<Context> context) const;
};

} // namespace v8

#endif // INCLUDE_V8_VALUE_H_
