/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_FUNCTION_H_
#define INCLUDE_V8_FUNCTION_H_

#include "v8-object.h"

namespace v8 {

class FunctionCallbackInfo;
typedef void (*FunctionCallback)(const FunctionCallbackInfo& info);

class FunctionCallbackInfo {
public:
    FunctionCallbackInfo(Isolate* isolate, Local<Object> holder, Local<Value> data,
                         int argc, Local<Value>* argv)
        : isolate_(isolate), holder_(holder), data_(data), argc_(argc), argv_(argv), return_value_() {}

    Isolate* GetIsolate() const { return isolate_; }
    int Length() const { return argc_; }
    Local<Value> operator[](int i) const {
        return (i >= 0 && i < argc_) ? argv_[i] : Local<Value>();
    }
    Local<Object> Holder() const { return holder_; }
    Local<Value> Data() const { return data_; }

    void SetReturnValue(Local<Value> value) { return_value_ = value; }
    Local<Value> GetReturnValue() const { return return_value_; }

private:
    Isolate* isolate_;
    Local<Object> holder_;
    Local<Value> data_;
    int argc_;
    Local<Value>* argv_;
    Local<Value> return_value_;
};

class Function : public Object {
public:
    static Local<Function> New(Local<Context> context, FunctionCallback callback,
                               Local<Value> data = Local<Value>(), int length = 0);
    Local<Value> Call(Local<Context> context, Local<Value> recv, int argc, Local<Value> argv[]);
    bool IsFunction() const override { return true; }
};

} // namespace v8

#endif // INCLUDE_V8_FUNCTION_H_
