/*
 * Copyright 2015 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_INTERPRETER_INTERPRETER_H_
#define V8_INTERPRETER_INTERPRETER_H_

#include "bytecodes.h"
#include "third_party/v8/src/heap/heap.h"

namespace v8 {
namespace internal {

class Interpreter {
public:
    Interpreter(Heap* heap);
    ~Interpreter();

    HeapObject* Execute(BytecodeArray* bytecode, JSObject* receiver = nullptr);

private:
    Heap* heap_;
    HeapObject* accumulator_;
    std::vector<HeapObject*> registers_;
};

} // namespace internal
} // namespace v8

#endif // V8_INTERPRETER_INTERPRETER_H_
