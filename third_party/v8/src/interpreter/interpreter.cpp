/*
 * Copyright 2015 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "interpreter.h"
#include "third_party/v8/src/codegen/x64/jit-compiler-x64.h"

namespace v8 {
namespace internal {

Interpreter::Interpreter(Heap* heap)
    : heap_(heap)
    , accumulator_(heap->GetUndefined()) {
    registers_.resize(32, heap->GetUndefined());
}

Interpreter::~Interpreter() {}

HeapObject* Interpreter::Execute(BytecodeArray* bytecode, JSObject* receiver) {
    (void)receiver;
    if (!bytecode) return heap_->GetUndefined();

    size_t ip = 0;
    const auto& instructions = bytecode->instructions;
    const auto& constants = bytecode->constants;

    while (ip < instructions.size()) {
        const Instruction& inst = instructions[ip++];

        switch (inst.op) {
            case Bytecode::kLdaZero:
                accumulator_ = heap_->AllocateNumber(0.0);
                break;

            case Bytecode::kLdaSmi:
                accumulator_ = heap_->AllocateNumber((double)inst.operand0);
                break;

            case Bytecode::kLdaConstant:
                if (inst.operand0 >= 0 && (size_t)inst.operand0 < constants.size()) {
                    accumulator_ = constants[inst.operand0];
                }
                break;

            case Bytecode::kLdar:
                if (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size()) {
                    accumulator_ = registers_[inst.operand0];
                }
                break;

            case Bytecode::kStar:
                if (inst.operand0 >= 0) {
                    if ((size_t)inst.operand0 >= registers_.size()) {
                        registers_.resize(inst.operand0 + 1, heap_->GetUndefined());
                    }
                    registers_[inst.operand0] = accumulator_;
                }
                break;

            case Bytecode::kAdd: {
                HeapObject* lhs = (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size())
                                  ? registers_[inst.operand0] : heap_->GetUndefined();
                HeapObject* rhs = accumulator_;

                if (lhs->type == HEAP_NUMBER_TYPE && rhs->type == HEAP_NUMBER_TYPE) {
                    double l = static_cast<JSNumber*>(lhs)->value;
                    double r = static_cast<JSNumber*>(rhs)->value;
                    accumulator_ = heap_->AllocateNumber(l + r);
                } else if (lhs->type == STRING_TYPE || rhs->type == STRING_TYPE) {
                    std::string l_str = (lhs->type == STRING_TYPE) ? static_cast<JSString*>(lhs)->value : "";
                    std::string r_str = (rhs->type == STRING_TYPE) ? static_cast<JSString*>(rhs)->value : "";
                    accumulator_ = heap_->AllocateString(l_str + r_str);
                }
                break;
            }

            case Bytecode::kSub: {
                HeapObject* lhs = (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size())
                                  ? registers_[inst.operand0] : heap_->GetUndefined();
                HeapObject* rhs = accumulator_;
                if (lhs->type == HEAP_NUMBER_TYPE && rhs->type == HEAP_NUMBER_TYPE) {
                    double l = static_cast<JSNumber*>(lhs)->value;
                    double r = static_cast<JSNumber*>(rhs)->value;
                    accumulator_ = heap_->AllocateNumber(l - r);
                }
                break;
            }

            case Bytecode::kMul: {
                HeapObject* lhs = (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size())
                                  ? registers_[inst.operand0] : heap_->GetUndefined();
                HeapObject* rhs = accumulator_;
                if (lhs->type == HEAP_NUMBER_TYPE && rhs->type == HEAP_NUMBER_TYPE) {
                    double l = static_cast<JSNumber*>(lhs)->value;
                    double r = static_cast<JSNumber*>(rhs)->value;
                    accumulator_ = heap_->AllocateNumber(l * r);
                }
                break;
            }

            case Bytecode::kDiv: {
                HeapObject* lhs = (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size())
                                  ? registers_[inst.operand0] : heap_->GetUndefined();
                HeapObject* rhs = accumulator_;
                if (lhs->type == HEAP_NUMBER_TYPE && rhs->type == HEAP_NUMBER_TYPE) {
                    double l = static_cast<JSNumber*>(lhs)->value;
                    double r = static_cast<JSNumber*>(rhs)->value;
                    accumulator_ = heap_->AllocateNumber(r != 0.0 ? (l / r) : 0.0);
                }
                break;
            }

            case Bytecode::kCallProperty: {
                // inst.operand0 is register holding function
                if (inst.operand0 >= 0 && (size_t)inst.operand0 < registers_.size()) {
                    HeapObject* target = registers_[inst.operand0];
                    if (target && target->type == JS_FUNCTION_TYPE) {
                        JSFunction* fn = static_cast<JSFunction*>(target);
                        // If JIT compiled, run native x86_64 code!
                        if (fn->jit_entry_point) {
                            typedef double (*JitFn)(double);
                            JitFn native_fn = (JitFn)fn->jit_entry_point;
                            double arg = (accumulator_->type == HEAP_NUMBER_TYPE)
                                         ? static_cast<JSNumber*>(accumulator_)->value : 0.0;
                            double res = native_fn(arg);
                            accumulator_ = heap_->AllocateNumber(res);
                        } else if (fn->bytecode) {
                            // Sub-interpreter invocation
                            Interpreter sub_vm(heap_);
                            sub_vm.registers_[0] = accumulator_; // Pass argument in reg 0
                            accumulator_ = sub_vm.Execute(fn->bytecode, fn);
                        }
                    }
                }
                break;
            }

            case Bytecode::kReturn:
                return accumulator_;

            default:
                break;
        }
    }

    return accumulator_;
}

} // namespace internal
} // namespace v8
