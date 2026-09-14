# ATRIX BROWSER: REALISTIC CHROMIUM INTEGRATION ROADMAP

**Document ID:** ATRIX-PHASE6-ROADMAP-001  
**Target:** New Revised Chromium-Based Engineering Roadmap for ATRIX  
**Date:** 2026-08-26  

---

## 1. Decommissioning Custom Engine Re-Invention

With the strategic decision to host mature Chromium components, the previous custom engine phases (Phase 6 Custom Layout, Phase 7 Custom Rasterizer, Phase 8 Custom Font Engine, Phase 9 Custom Image Pipeline, Phase 10 Custom JS Engine) are **SUPERSEDED AND CONSOLIDATED** into a unified, dependency-ordered **Chromium Platform Bring-Up Roadmap**.

---

## 2. Revised 7-Stage Integration Roadmap

```
Stage 1: Platform Runtime (POSIX / C++ Userspace Foundation)
   ↓
Stage 2: Toolchain & Cross-Build Environment (GN / Ninja / Clang)
   ↓
Stage 3: Skia 2D Software Renderer ➔ BWE Window Surface Bridge
   ↓
Stage 4: V8 JavaScript & WebAssembly Engine Bring-Up
   ↓
Stage 5: Blink Web Platform (HTML5, DOM, CSSOM, Layout) Integration
   ↓
Stage 6: Chromium Content Embedder & ATRIX Omnibox Unification
   ↓
Stage 7: Chromium-Class Browser Certification & Hardening
```

---

## 3. Detailed Stage Breakdown & Effort Assessment

### Stage 1 — Userspace Platform Runtime (`musl` + `libc++` + Syscalls)
- **Scope:** Implement POSIX memory syscalls (`sys_mmap`, `sys_munmap`, `sys_mprotect`), synchronization syscalls (`sys_futex`), and file descriptor tables. Port lightweight `musl libc` + LLVM `libc++` headers.
- **Estimated Effort:** **HIGH** (2-3 weeks engineering effort).
- **Deliverable:** Working user-mode C++20 standard library capable of dynamic memory allocation and multithreading.

### Stage 2 — Toolchain & Cross-Build Environment
- **Scope:** Configure GN (Generate Ninja) toolchain files (`toolchain("atoms_x64")`) and Python code-generation scripts.
- **Estimated Effort:** **MEDIUM** (1-2 weeks engineering effort).
- **Deliverable:** Automated command line build producing statically/dynamically linked ELF binaries for ATOMS OS.

### Stage 3 — Skia 2D Graphics ➔ BWE Integration
- **Scope:** Build Skia CPU software rasterizer (`SkBitmap`/`SkCanvas`) and create the `APAL_SkiaBlitter` bridge blitting directly into ATOMS BWE window double-buffered surfaces.
- **Estimated Effort:** **LOW** (3-5 days engineering effort).
- **Deliverable:** High-performance 2D vector, text, and image rasterization rendered smoothly in native ATOMS windows.

### Stage 4 — V8 JavaScript & WebAssembly Engine Bring-Up
- **Scope:** Configure V8 for ATOMS x86_64 target with `v8::PageAllocator` wired to `sys_mmap`. Validate Ignition bytecode execution and JIT code compilation.
- **Estimated Effort:** **HIGH** (2-3 weeks engineering effort).
- **Deliverable:** Full ECMAScript 2024 compliance executing complex JavaScript web apps in ATOMS userspace.

### Stage 5 — Blink Web Platform Engine Integration
- **Scope:** Vendor and compile Blink HTML5 parser, DOM core, CSS3 cascade, and Layout engine. Wire Skia as painting backend and V8 as JS runtime.
- **Estimated Effort:** **VERY HIGH** (4-6 weeks engineering effort).
- **Deliverable:** Complete web engine processing arbitrary real-world web pages into interactive layout trees.

### Stage 6 — Chromium Content Embedder & ATRIX Omnibox Unification
- **Scope:** Create the native ATRIX Browser embedder shell. Wire Omnibox URL navigation, back/forward history, scrollbars, and input event dispatching.
- **Estimated Effort:** **MEDIUM** (1-2 weeks engineering effort).
- **Deliverable:** Fully unified native ATRIX Browser GUI powered by Chromium web engine.

### Stage 7 — Chromium-Class Browser Certification
- **Scope:** Execute W3C Web Platform Tests (WPT), Acid3 tests, and live real-world navigation against top public websites (Google, Wikipedia, GitHub).
- **Estimated Effort:** **MEDIUM** (1 week verification effort).
- **Deliverable:** Formal Chromium-Class Milestone Certification.
