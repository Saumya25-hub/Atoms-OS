/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_OBJECT_H_
#define INCLUDE_V8_OBJECT_H_

#include "v8-value.h"

namespace v8 {

class Object : public Value {
public:
    static Local<Object> New(Isolate* isolate);

    bool Set(Local<Context> context, Local<Value> key, Local<Value> value);
    bool Set(Local<Context> context, uint32_t index, Local<Value> value);
    Local<Value> Get(Local<Context> context, Local<Value> key);
    Local<Value> Get(Local<Context> context, uint32_t index);
    bool Has(Local<Context> context, Local<Value> key);
    bool Delete(Local<Context> context, Local<Value> key);

    Local<Array> GetPropertyNames(Local<Context> context);

    bool IsObject() const override { return true; }
};

} // namespace v8

#endif // INCLUDE_V8_OBJECT_H_
