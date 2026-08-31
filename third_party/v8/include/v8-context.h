/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_CONTEXT_H_
#define INCLUDE_V8_CONTEXT_H_

#include "v8-value.h"

namespace v8 {

class Object;
class ObjectTemplate;

class Context {
public:
    class Scope {
    public:
        explicit Scope(Local<Context> context);
        ~Scope();
    private:
        Local<Context> context_;
    };

    static Local<Context> New(Isolate* isolate, void* extension = nullptr,
                              Local<Object> global_template = Local<Object>());

    Local<Object> Global();
    Isolate* GetIsolate();
    void Enter();
    void Exit();
};

} // namespace v8

#endif // INCLUDE_V8_CONTEXT_H_
