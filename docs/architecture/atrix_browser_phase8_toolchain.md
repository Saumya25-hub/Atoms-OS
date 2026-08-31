# ATRIX Browser — Phase 8: Chromium GN/Ninja + Clang Toolchain Architecture

**Document ID:** ATRIX-ARCH-PHASE8-001  
**Phase:** Phase 8 — Toolchain & Meta-Build Architecture  
**Status:** Certified & Production Baseline  
**Date:** 2026-08-26  

---

## 1. Overview & Architectural Principles

Phase 8 establishes the cross-compilation and build pipeline for ATOMS OS using the official Google Chromium GN meta-build generator, Ninja build tool, and Clang/LLVM 22.1.8 compiler suite.

The toolchain architecture enforces:
1. **Open-Source Standard Alignment:** Retains official GN syntax and toolchain conventions (`tool("cc")`, `tool("cxx")`, `tool("alink")`, `tool("link")`).
2. **Two-Stage Toolchain Separation:**
   - **Host Toolchain (`//gn/toolchains/host:host_x86_64`):** Compiles host-side build generators, IDL parsers, and resource packagers for the development host.
   - **Target Toolchain (`//gn/toolchains/atoms:atoms_x86_64`):** Cross-compiles C11 and C++20 source files to freestanding x86_64 ELF binaries linking against the Phase 7 ATOMS userspace runtime.
3. **Reproducibility & Determinism:** 1-command build invocation via `tools/gn_build.ps1` (`gn gen out/Default && ninja -C out/Default`).

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

## 2. Directory Layout & Build Hierarchy

```text
D:/Signatures_OS/
├── .gn                                    # Root GN configuration (points to BUILDCONFIG.gn)
├── BUILD.gn                               # Root build file declaring all targets
├── gn/
│   ├── BUILDCONFIG.gn                     # Target/Host OS, CPU, and default configs
│   ├── config/
│   │   └── BUILD.gn                       # Compiler & linker flags, include paths, defines
│   └── toolchains/
│       ├── atoms/
│       │   └── BUILD.gn                   # Target toolchain definition (clang/clang++/llvm-ar/ld.lld)
│       └── host/
│           └── BUILD.gn                   # Host toolchain definition (Windows host tools)
├── tools/
│   ├── gn.exe                             # Official Google GN binary (v2531)
│   ├── ninja.exe                          # Official Ninja build binary (v1.13.0)
│   ├── gn_build.ps1                       # Automated build driver script
│   └── codegen/
│       └── test_code_generator.py         # Host Python build action script
├── third_party/
│   └── chromium_probe/                    # Chromium base and Abseil progressive build probe
│       ├── chromium_base_probe.h / .cpp
│       ├── absl_probe.h / .cpp
│       └── probe_main.cpp
└── out/
    └── Default/                           # Generated Ninja files and output binaries
        ├── build.ninja
        ├── toolchain.ninja
        ├── atoms_c_test.elf
        ├── atoms_cpp_test.elf
        ├── atoms_generated_test.elf
        └── chromium_toolchain_verify.elf
```

---

## 3. Provenance & License Summary

- **GN Binary & Concepts:** BSD 3-Clause License (The Chromium Authors / Google LLC)
- **Ninja Engine:** Apache 2.0 License (Evan Martin / Ninja Project)
- **LLVM / Clang:** Apache 2.0 with LLVM Exception (The LLVM Project)
- **Chromium Base & Abseil Primitives:** BSD 3-Clause / Apache 2.0 License
- **ATOMS OS Integration Glue & Scripts:** Proprietary / ATOMS OS Team

See [`ATOMS_THIRDPARTY_TOOLCHAIN_LICENSES.md`](file:///D:/Signatures_OS/ATOMS_THIRDPARTY_TOOLCHAIN_LICENSES.md) and [`ATOMS_TOOLCHAIN_PROVENANCE.md`](file:///D:/Signatures_OS/ATOMS_TOOLCHAIN_PROVENANCE.md) for full attribution records.
