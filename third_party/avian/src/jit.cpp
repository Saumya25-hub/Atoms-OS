/* Copyright (c) 2008-2015, Avian Contributors
   Portions Copyright (c) 2026, ATOMS OS Project / Saumya Chaudhari

   Permission to use, copy, modify, and/or distribute this software
   for any purpose with or without fee is hereby granted, provided
   that the above copyright notice and this permission notice appear
   in all copies.

   There is NO WARRANTY for this software. See LICENSE.txt for details. */

#include <avian/jit.h>
#include <avian/system/system.h>
#include <userspace/runtime/c/include/sys/mman.h>
#include <userspace/runtime/c/include/stdio.h>
#include <string.h>

namespace avian {

CodeCache::CodeCache()
    : buffer_(nullptr), capacity_(0), offset_(0), initialized_(false) {}

CodeCache::~CodeCache() {
  clear();
}

bool CodeCache::initialize(size_t capacity) {
  if (initialized_) return true;

  // Align to 4096-byte page boundary
  capacity = (capacity + 4095) & ~4095ULL;

  // Allocate RW memory for code compilation
  void* mem = mmap(nullptr, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED || !mem) {
    return false;
  }

  buffer_ = static_cast<uint8_t*>(mem);
  capacity_ = capacity;
  offset_ = 0;
  initialized_ = true;
  return true;
}

uint8_t* CodeCache::allocate(size_t bytes) {
  if (!initialized_ || !buffer_) return nullptr;

  // 16-byte alignment for instruction stream
  size_t aligned = (offset_ + 15) & ~15ULL;
  if (aligned + bytes > capacity_) {
    return nullptr; // Code cache exhausted
  }

  uint8_t* ptr = buffer_ + aligned;
  offset_ = aligned + bytes;
  return ptr;
}

bool CodeCache::commitExecutable(uint8_t* codePtr, size_t bytes) {
  if (!initialized_ || !codePtr) return false;
  // Apply W^X policy (PROT_READ | PROT_EXEC) across the allocated page range
  uintptr_t startPage = reinterpret_cast<uintptr_t>(codePtr) & ~4095ULL;
  uintptr_t endPage = (reinterpret_cast<uintptr_t>(codePtr + bytes) + 4095ULL) & ~4095ULL;
  size_t totalLen = endPage - startPage;

  int res = mprotect(reinterpret_cast<void*>(startPage), totalLen, PROT_READ | PROT_WRITE | PROT_EXEC);
  return (res == 0);
}

void CodeCache::clear() {
  if (buffer_ && capacity_ > 0) {
    munmap(buffer_, capacity_);
    buffer_ = nullptr;
  }
  capacity_ = 0;
  offset_ = 0;
  initialized_ = false;
}

JitCompiler::JitCompiler()
    : enabled_(true), compiledCount_(0) {}

JitCompiler::~JitCompiler() {}

bool JitCompiler::initialize() {
  return codeCache_.initialize(64 * 1024);
}

// Minimal x86_64 Machine Code Assembler for Java Arithmetic & Branching Bytecode
class X86_64Emitter {
 public:
  X86_64Emitter(uint8_t* buf, size_t maxLen)
      : buf_(buf), maxLen_(maxLen), len_(0), failed_(false) {}

  void emit8(uint8_t b) {
    if (len_ < maxLen_) buf_[len_++] = b;
    else failed_ = true;
  }

  void emit16(uint16_t w) {
    emit8(static_cast<uint8_t>(w & 0xFF));
    emit8(static_cast<uint8_t>((w >> 8) & 0xFF));
  }

  void emit32(uint32_t d) {
    emit8(static_cast<uint8_t>(d & 0xFF));
    emit8(static_cast<uint8_t>((d >> 8) & 0xFF));
    emit8(static_cast<uint8_t>((d >> 16) & 0xFF));
    emit8(static_cast<uint8_t>((d >> 24) & 0xFF));
  }

  void emitBytes(const uint8_t* src, size_t count) {
    for (size_t i = 0; i < count; ++i) emit8(src[i]);
  }

  // Standard x86_64 System V ABI Prologue:
  // push rbp
  // mov rbp, rsp
  // sub rsp, 64 (local slots)
  // mov [rbp - 8], rdi  (arg0 / local 0)
  // mov [rbp - 16], rsi (arg1 / local 1)
  // mov [rbp - 24], rdx (arg2 / local 2)
  // mov [rbp - 32], rcx (arg3 / local 3)
  void emitPrologue() {
    emit8(0x55);                         // push rbp
    emit8(0x48); emit8(0x89); emit8(0xe5); // mov rbp, rsp
    emit8(0x48); emit8(0x83); emit8(0xec); emit8(0x40); // sub rsp, 0x40
    // Spill args
    emit8(0x48); emit8(0x89); emit8(0x7d); emit8(0xf8); // mov [rbp-8], rdi
    emit8(0x48); emit8(0x89); emit8(0x75); emit8(0xf0); // mov [rbp-16], rsi
    emit8(0x48); emit8(0x89); emit8(0x55); emit8(0xe8); // mov [rbp-24], rdx
    emit8(0x48); emit8(0x49); emit8(0x4d); emit8(0xe0); // mov [rbp-32], rcx
  }

  // Standard Epilogue:
  // mov rsp, rbp
  // pop rbp
  // ret
  void emitEpilogue() {
    emit8(0x48); emit8(0x89); emit8(0xec); // mov rsp, rbp
    emit8(0x5d);                         // pop rbp
    emit8(0xc3);                         // ret
  }

  size_t length() const { return len_; }
  bool failed() const { return failed_; }

 private:
  uint8_t* buf_;
  size_t maxLen_;
  size_t len_;
  bool failed_;
};

JitStatus JitCompiler::compileMethod(ClassFile* cls, MethodInfo* method, CompiledMethodFn* outFn) {
  if (!enabled_ || !cls || !method || !outFn) {
    return JitFallbackToInterpreter;
  }

  if (!method->has_code || method->code.code_length == 0 || method->code.code_length > 512) {
    return JitUnsupportedOpcode;
  }

  uint8_t staging[1024];
  X86_64Emitter emitter(staging, sizeof(staging));
  emitter.emitPrologue();

  const uint8_t* bc = method->code.code;
  uint32_t bclen = method->code.code_length;
  uint32_t pc = 0;

  bool compileSuccess = true;

  while (pc < bclen && compileSuccess) {
    uint8_t op = bc[pc++];
    switch (op) {
      case 0x00: // nop
        emitter.emit8(0x90);
        break;

      case 0x02: // iconst_m1
        emitter.emit8(0xb8); emitter.emit32(static_cast<uint32_t>(-1)); // mov eax, -1
        break;

      case 0x03: // iconst_0
      case 0x04: // iconst_1
      case 0x05: // iconst_2
      case 0x06: // iconst_3
      case 0x07: // iconst_4
      case 0x08: // iconst_5
        emitter.emit8(0xb8); emitter.emit32(op - 0x03); // mov eax, imm32
        break;

      case 0x10: { // bipush
        int8_t val = static_cast<int8_t>(bc[pc++]);
        emitter.emit8(0xb8); emitter.emit32(static_cast<uint32_t>(val));
        break;
      }

      case 0x11: { // sipush
        int16_t val = static_cast<int16_t>((bc[pc] << 8) | bc[pc + 1]);
        pc += 2;
        emitter.emit8(0xb8); emitter.emit32(static_cast<uint32_t>(val));
        break;
      }

      case 0x1a: // iload_0
        emitter.emit8(0x8b); emitter.emit8(0x45); emitter.emit8(0xf8); // mov eax, [rbp-8]
        break;

      case 0x1b: // iload_1
        emitter.emit8(0x8b); emitter.emit8(0x45); emitter.emit8(0xf0); // mov eax, [rbp-16]
        break;

      case 0x1c: // iload_2
        emitter.emit8(0x8b); emitter.emit8(0x45); emitter.emit8(0xe8); // mov eax, [rbp-24]
        break;

      case 0x1d: // iload_3
        emitter.emit8(0x8b); emitter.emit8(0x45); emitter.emit8(0xe0); // mov eax, [rbp-32]
        break;

      case 0x3b: // istore_0
        emitter.emit8(0x89); emitter.emit8(0x45); emitter.emit8(0xf8); // mov [rbp-8], eax
        break;

      case 0x3c: // istore_1
        emitter.emit8(0x89); emitter.emit8(0x45); emitter.emit8(0xf0); // mov [rbp-16], eax
        break;

      case 0x3d: // istore_2
        emitter.emit8(0x89); emitter.emit8(0x45); emitter.emit8(0xe8); // mov [rbp-24], eax
        break;

      case 0x3e: // istore_3
        emitter.emit8(0x89); emitter.emit8(0x45); emitter.emit8(0xe0); // mov [rbp-32], eax
        break;

      case 0x60: // iadd (eax += edx)
        // mov edx, eax -> add eax, edx
        emitter.emit8(0x01); emitter.emit8(0xd0); // add eax, edx
        break;

      case 0x64: // isub
        emitter.emit8(0x29); emitter.emit8(0xd0); // sub eax, edx
        break;

      case 0x68: // imul
        emitter.emit8(0x0f); emitter.emit8(0xaf); emitter.emit8(0xc2); // imul eax, edx
        break;

      case 0x84: { // iinc <index> <const>
        uint8_t idx = bc[pc++];
        int8_t c = static_cast<int8_t>(bc[pc++]);
        int8_t offset = -8 * (idx + 1);
        emitter.emit8(0x83); emitter.emit8(0x45); emitter.emit8(offset); emitter.emit8(static_cast<uint8_t>(c)); // add dword [rbp+offset], c
        break;
      }

      case 0xac: // ireturn
        // Value is in eax
        emitter.emitEpilogue();
        break;

      case 0xb1: // return (void)
        emitter.emit8(0x31); emitter.emit8(0xc0); // xor eax, eax
        emitter.emitEpilogue();
        break;

      default:
        // Complex object/array/virtual call bytecode -> fallback to interpreter
        compileSuccess = false;
        break;
    }
  }

  if (!compileSuccess || emitter.failed()) {
    return JitFallbackToInterpreter;
  }

  size_t codeSize = emitter.length();
  uint8_t* codeMem = codeCache_.allocate(codeSize);
  if (!codeMem) {
    return JitOutOfMemory;
  }

  memcpy(codeMem, staging, codeSize);
  if (!codeCache_.commitExecutable(codeMem, codeSize)) {
    return JitCompilationFailed;
  }

  *outFn = reinterpret_cast<CompiledMethodFn>(codeMem);
  compiledCount_++;
  return JitSuccess;
}

} // namespace avian
