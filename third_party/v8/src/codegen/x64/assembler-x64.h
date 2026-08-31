/*
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2026 The V8 Authors / ATOMS OS Adaptation
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef V8_CODEGEN_X64_ASSEMBLER_X64_H_
#define V8_CODEGEN_X64_ASSEMBLER_X64_H_

#include <stdint.h>
#include <stddef.h>
#include "userspace/runtime/cpp/include/vector"

namespace v8 {
namespace internal {

class AssemblerX64 {
public:
    AssemblerX64();
    ~AssemblerX64();

    void EmitByte(uint8_t b);
    void EmitBytes(const uint8_t* data, size_t len);

    // x86_64 ABI code generation primitives
    void PushRbp();
    void MovRbpRsp();
    void PopRbp();
    void Ret();

    // Floating-point math (xmm0 = xmm0 * xmm0, etc.)
    void MulsdXmm0Xmm0(); // Square: xmm0 * xmm0 -> xmm0
    void AddsdXmm0Xmm1(); // xmm0 + xmm1 -> xmm0

    // Integer math
    void MovRaxRdi();     // mov %rdi, %rax
    void ImulRaxRsi();    // imul %rsi, %rax
    void AddRaxRsi();     // add %rsi, %rax

    const uint8_t* GetBuffer() const { return buffer_.data(); }
    size_t GetSize() const { return buffer_.size(); }

private:
    std::vector<uint8_t> buffer_;
};

} // namespace internal
} // namespace v8

#endif // V8_CODEGEN_X64_ASSEMBLER_X64_H_
