/*
 * Copyright 2015 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_INTERPRETER_BYTECODES_H_
#define V8_INTERPRETER_BYTECODES_H_

#include <stdint.h>
#include <stddef.h>
#include "userspace/runtime/cpp/include/vector"
#include "third_party/v8/src/objects/objects.h"

namespace v8 {
namespace internal {

enum class Bytecode : uint8_t {
    kLdaZero,
    kLdaSmi,
    kLdaConstant,
    kLdar,
    kStar,
    kAdd,
    kSub,
    kMul,
    kDiv,
    kMod,
    kTestEqual,
    kTestLessThan,
    kTestGreaterThan,
    kJump,
    kJumpIfTrue,
    kJumpIfFalse,
    kCallProperty,
    kReturn,
    kLastBytecode = kReturn
};

struct Instruction {
    Bytecode op;
    int32_t  operand0;
    int32_t  operand1;
};

class BytecodeArray {
public:
    std::vector<Instruction> instructions;
    std::vector<HeapObject*> constants;
    int register_count;

    BytecodeArray() : register_count(16) {}

    void Emit(Bytecode op, int32_t op0 = 0, int32_t op1 = 0) {
        instructions.push_back({op, op0, op1});
    }

    int AddConstant(HeapObject* obj) {
        constants.push_back(obj);
        return (int)constants.size() - 1;
    }
};

} // namespace internal
} // namespace v8

#endif // V8_INTERPRETER_BYTECODES_H_
