/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_HANDLE_SCOPE_H_
#define INCLUDE_V8_HANDLE_SCOPE_H_

#include "v8-value.h"

namespace v8 {

class Isolate;

class HandleScope {
public:
    explicit HandleScope(Isolate* isolate);
    ~HandleScope();

    static int NumberOfHandles(Isolate* isolate);

    template <class T>
    Local<T> CloseAndEscape(Local<T> value_escape) {
        return value_escape;
    }

private:
    Isolate* isolate_;
    void* prev_scope_;
};

class EscapableHandleScope : public HandleScope {
public:
    explicit EscapableHandleScope(Isolate* isolate) : HandleScope(isolate) {}
    template <class T>
    Local<T> Escape(Local<T> value) {
        return value;
    }
};

} // namespace v8

#endif // INCLUDE_V8_HANDLE_SCOPE_H_
