/*
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "assembler-x64.h"

namespace v8 {
namespace internal {

AssemblerX64::AssemblerX64() {}
AssemblerX64::~AssemblerX64() {}

void AssemblerX64::EmitByte(uint8_t b) {
    buffer_.push_back(b);
}

void AssemblerX64::EmitBytes(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer_.push_back(data[i]);
    }
}

void AssemblerX64::PushRbp() {
    EmitByte(0x55); // push %rbp
}

void AssemblerX64::MovRbpRsp() {
    static const uint8_t opcodes[] = {0x48, 0x89, 0xE5}; // mov %rsp, %rbp
    EmitBytes(opcodes, sizeof(opcodes));
}

void AssemblerX64::PopRbp() {
    EmitByte(0x5D); // pop %rbp
}

void AssemblerX64::Ret() {
    EmitByte(0xC3); // ret
}

void AssemblerX64::MulsdXmm0Xmm0() {
    // mulsd %xmm0, %xmm0 (F2 0F 59 C0)
    static const uint8_t opcodes[] = {0xF2, 0x0F, 0x59, 0xC0};
    EmitBytes(opcodes, sizeof(opcodes));
}

void AssemblerX64::AddsdXmm0Xmm1() {
    // addsd %xmm1, %xmm0 (F2 0F 58 C1)
    static const uint8_t opcodes[] = {0xF2, 0x0F, 0x58, 0xC1};
    EmitBytes(opcodes, sizeof(opcodes));
}

void AssemblerX64::MovRaxRdi() {
    // mov %rdi, %rax (48 89 F8)
    static const uint8_t opcodes[] = {0x48, 0x89, 0xF8};
    EmitBytes(opcodes, sizeof(opcodes));
}

void AssemblerX64::ImulRaxRsi() {
    // imul %rsi, %rax (48 0F AF C6)
    static const uint8_t opcodes[] = {0x48, 0x0F, 0xAF, 0xC6};
    EmitBytes(opcodes, sizeof(opcodes));
}

void AssemblerX64::AddRaxRsi() {
    // add %rsi, %rax (48 01 F0)
    static const uint8_t opcodes[] = {0x48, 0x01, 0xF0};
    EmitBytes(opcodes, sizeof(opcodes));
}

} // namespace internal
} // namespace v8
