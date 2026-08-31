/*
 * Copyright 2014 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_API_API_H_
#define V8_API_API_H_

#include "third_party/v8/include/v8.h"
#include "third_party/v8/src/heap/heap.h"
#include "third_party/v8/src/interpreter/interpreter.h"

namespace v8 {

class IsolateImpl : public Isolate {
public:
    IsolateImpl(Platform* platform);
    ~IsolateImpl();

    internal::Heap* heap() { return &heap_; }
    internal::Interpreter* interpreter() { return &interpreter_; }
    Platform* platform() { return platform_; }

private:
    Platform* platform_;
    internal::Heap heap_;
    internal::Interpreter interpreter_;
};

} // namespace v8

#endif // V8_API_API_H_
