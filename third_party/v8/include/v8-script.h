/*
 * Copyright 2021 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef INCLUDE_V8_SCRIPT_H_
#define INCLUDE_V8_SCRIPT_H_

#include "v8-value.h"

namespace v8 {

class ScriptOrigin {
public:
    ScriptOrigin(Local<Value> resource_name = Local<Value>())
        : resource_name_(resource_name) {}
    Local<Value> ResourceName() const { return resource_name_; }
private:
    Local<Value> resource_name_;
};

class Script {
public:
    static Local<Script> Compile(Local<Context> context, Local<String> source,
                                 ScriptOrigin* origin = nullptr);

    Local<Value> Run(Local<Context> context);
};

} // namespace v8

#endif // INCLUDE_V8_SCRIPT_H_
