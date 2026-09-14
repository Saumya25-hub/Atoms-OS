# PHASE 8 PATCH PLAN: CHROMIUM GN/NINJA + CLANG TOOLCHAIN FOUNDATION

**Document ID:** ATRIX-PHASE8-PLAN-001  
**Phase:** TASK 3 — ARCHITECT TEAM  
**Target Subsystem:** Chromium Toolchain, GN/Ninja Pipeline, Clang/LLVM Cross-Compilation & Target Platform Identity  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Scope & Target Pipeline

Phase 8 builds the official GN + Ninja + Clang cross-compilation pipeline for ATOMS OS. The architecture bridges Chromium meta-build standards with ATOMS kernel and userspace runtime interfaces:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                          DEVELOPER / BUILD INVOCATION                  │
│                        (gn gen out/Default && ninja)                   │
├────────────────────────────────────────────────────────────────────────┤
│                       GN (Generate Ninja Meta-Build)                   │
│         (.gn -> gn/BUILDCONFIG.gn -> BUILD.gn -> .ninja files)         │
├───────────────────────────────────┬────────────────────────────────────┤
│         Host Toolchain            │        ATOMS Target Toolchain      │
│      (toolchain/host_x86_64)      │       (toolchain/atoms_x86_64)     │
│   • Python / C++ code generator   │  • Clang x86_64 freestanding       │
│   • Resource / IDL compilers      │  • Phase 7 libc/libc++ headers     │
│   • Host-executable binaries      │  • LLD ELF64 linker + linker script│
├───────────────────────────────────┴────────────────────────────────────┤
│                     Ninja Build Execution (out/Default)                │
│             (Compiles objects, static libraries, executables)          │
├────────────────────────────────────────────────────────────────────────┤
│                   ATOMS OS Userspace Executable Output                 │
│               (ELF64 binaries compatible with ATOMS Loader)            │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. ATOMS Target Platform Identity (Task 3 Decisions)

| Property | Decision / Value | Rationale |
|:---|:---|:---|
| **Target OS Name (`target_os`)** | `"atoms"` | Distinguishes ATOMS OS from Linux, Windows, or Darwin in GN conditionals. |
| **Target CPU (`target_cpu`)** | `"x86_64"` | Intel 64-bit architecture (Haswell Core i3 / QEMU x86_64). |
| **Endianness** | Little-Endian | Native x86_64 architecture standard. |
| **Object File Format** | ELF64 (`-target x86_64-pc-none-elf`) | Standard for ATOMS userspace binaries and ELF loaders. |
| **Executable Linker** | `ld.lld` | High-performance LLVM linker compatible with GNU linker scripts. |
| **Linker Script** | `userspace/linker.ld` | Places `.text` at `0x01000000` with 4KB page alignment. |
| **C Standard** | C11 / C17 (`-std=c11`) | Standard POSIX / C library standard. |
| **C++ Standard** | C++20 (`-std=c++20`) | Required by modern Chromium, Blink, V8, and Skia codebases. |
| **C++ ABI Settings** | `-fno-exceptions`, `-fno-rtti` | Matches standard Chromium embedded configuration. |
| **Threading Model** | POSIX Threads (`pthread`) | Backed by ATOMS kernel `SYS_FUTEX` and `SYS_THREAD_SPAWN`. |
| **Memory Allocation** | Dynamic Heap (`malloc`/`free`) | Backed by ATOMS kernel `SYS_MMAP` / `SYS_MUNMAP` anonymous pages. |

---

## 3. Files to Create and Modify

### 3.1 GN Build Configuration Infrastructure
- **Create:** `.gn`: Root GN configuration file designating `//gn/BUILDCONFIG.gn`.
- **Create:** `gn/BUILDCONFIG.gn`: Target/host OS definition, default toolchain routing, and target defaults.
- **Create:** `gn/config/BUILD.gn`: Compiler/linker configs (`atoms_target_flags`, `atoms_include_dirs`, `atoms_defines`).
- **Create:** `gn/toolchains/atoms/BUILD.gn`: GN toolchain definition for ATOMS target (`cc`, `cxx`, `asm`, `alink`, `link`, `stamp`, `copy`).
- **Create:** `gn/toolchains/host/BUILD.gn`: GN toolchain definition for Windows host generators.
- **Create:** `BUILD.gn`: Root build file declaring all verification and Chromium probe targets.

### 3.2 Automated Driver & Generator Tools
- **Create:** `tools/gn_build.ps1`: Script orchestrating GN generation and Ninja build execution.
- **Create:** `tools/codegen/test_code_generator.py`: Python host code generator proving the host action pipeline.

### 3.3 Chromium Probe & Compatibility Verification Targets
- **Create:** `third_party/chromium_probe/chromium_base_probe.h` & `.cpp`: Demonstrates Level 2 Chromium base primitives (`TimeTicks`, `Span`, `ObserverList`, atomics).
- **Create:** `third_party/chromium_probe/absl_probe.h` & `.cpp`: Demonstrates Level 3 Abseil foundational types (`string_view`, `Status`, `Span`).
- **Create:** `userspace/tests/toolchain_test/atoms_c_test.c`: C11 test target linking against Phase 7 libc.
- **Create:** `userspace/tests/toolchain_test/atoms_cpp_test.cpp`: C++20 test target linking against Phase 7 libc++.

---

## 4. Risk Assessment & Rollback Plan

- **Risk:** GN toolchain misconfiguration causing syntax errors in generated `.ninja` files.
  - **Mitigation:** Strict conformance to GN specification; tested immediately via `tools/gn.exe gen out/Default` and `tools/ninja.exe -C out/Default`.
- **Risk:** Host vs Target binary confusion.
  - **Mitigation:** Separate `toolchain("atoms_x86_64")` and `toolchain("host_x86_64")` with explicit command strings.
- **Rollback:** GN files reside in dedicated directories (`gn/`, `out/`) and do not modify core kernel files.
