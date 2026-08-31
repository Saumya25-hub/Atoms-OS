/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_EXCEPTION_H_
#define INCLUDE_V8_EXCEPTION_H_

#include "v8-value.h"

namespace v8 {

class TryCatch {
public:
    explicit TryCatch(Isolate* isolate);
    ~TryCatch();

    bool HasCaught() const;
    Local<Value> Exception() const;
    Local<Value> StackTrace(Local<Context> context) const;
    Local<String> Message() const;
    void Reset();

private:
    Isolate* isolate_;
    void* prev_try_catch_;
    Local<Value> exception_;
    Local<String> message_;
    bool has_caught_;
};

class Exception {
public:
    static Local<Value> Error(Local<String> message);
    static Local<Value> TypeError(Local<String> message);
    static Local<Value> RangeError(Local<String> message);
    static Local<Value> ReferenceError(Local<String> message);
    static Local<Value> SyntaxError(Local<String> message);
};

} // namespace v8

#endif // INCLUDE_V8_EXCEPTION_H_
