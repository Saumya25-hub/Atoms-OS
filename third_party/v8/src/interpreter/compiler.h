/*
 * Copyright 2015 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_INTERPRETER_COMPILER_H_
#define V8_INTERPRETER_COMPILER_H_

#include "bytecodes.h"
#include "third_party/v8/src/heap/heap.h"
#include "userspace/runtime/cpp/include/string"

namespace v8 {
namespace internal {

class Compiler {
public:
    static BytecodeArray* CompileScript(Heap* heap, const std::string& source);
};

} // namespace internal
} // namespace v8

#endif // V8_INTERPRETER_COMPILER_H_
