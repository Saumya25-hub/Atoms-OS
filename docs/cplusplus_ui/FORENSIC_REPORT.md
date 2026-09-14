# FORENSIC REPORT — BOS C++ UI FOUNDATION AUDIT

**Target:** ATOMS OS Native Userspace C/C++ UI Framework  
**Scope:** Phase 1 Foundation Architecture & C ABI Boundary Inspection  
**Date:** 2026-09-10  
**Phase:** TASK 1 — FORENSIC TEAM (NO CODE MODIFIED)

---

## 1. Executive Summary & Root Architecture

ATOMS OS currently features a layered, authoritative C-based windowing and composition engine located in kernel space (`kernel/wm/surface/surface.c`, `kernel/wm/bwe/`, `kernel/wm/compositor/`). Userspace applications interact with this subsystem through frozen hardware system calls (Syscalls 16–23).

While userspace contains:
1. Procedural C GUI bindings (`userspace/libbos_gui/`),
2. APAL platform adapters (`atoms/userspace/apal/`),
3. A fully functional freestanding C++20 runtime environment (`atoms/userspace/runtime/libatoms_cpp.a` with LLVM libc++ / libc++abi),

it has lacked an authoritative, production-grade, object-oriented C++ application/UI framework. Previous attempts in `sdk/include/bosui` were minimal prototypes targeting mock in-memory platform structures (`platform/`) rather than real userspace surface mapping and native event dispatch.

---

## 2. Forensic Evidence & Subsystem Analysis

### 2.1 Kernel Window Server & Compositor
- **File:** `kernel/wm/surface/surface.c`, `kernel/wm/surface/surface.h`
- **Mechanism:**
  - Manages `BWE_Surface` pool (`BWE_MAX_SURFACES = 64`).
  - Implements window dragging, active focus, minimize, maximize, titlebar chrome, and atomic frame composition (`BOHeart_Pulse`, `BOF_ComposeFullFrame`).
- **Verdict:** Authoritative native window manager. The C++ framework must NOT create a competing window manager; it must interface directly with this system.

### 2.2 Frozen GUI Syscall ABI (16–23)
- **Files:** `userspace/libbos_gui/include/syscalls_gui.h`, `userspace/libbos_gui/src/syscalls_gui.c`, `atoms/userspace/runtime/include/atoms_syscall.h`
- **Syscall Inventory:**
  - `SYS_GUI_CREATE_WINDOW` (16): Creates native window in kernel compositor.
  - `SYS_GUI_DESTROY_WINDOW` (17): Closes window and frees surface resources.
  - `SYS_GUI_SHOW_WINDOW` (18): Controls visibility.
  - `SYS_GUI_SET_BOUNDS` (19): Updates position and dimensions.
  - `SYS_GUI_MAP_SURFACE` (20): Directly maps the 32-bpp window framebuffer into userspace memory (`uint32_t** out_surface_pixels`, `uint32_t* out_stride_bytes`).
  - `SYS_GUI_INVALIDATE` (21): Marks damaged rectangles for composite and hardware refresh.
  - `SYS_GUI_POLL_EVENT` (22): Dequeues `BOS_GUIEvent` instances (mouse move, button down/up, key down/up, close).
  - `SYS_GUI_GET_SCREEN_INFO` (23): Fetches resolution and color depth.
- **Verdict:** This is the rock-solid, frozen C ABI boundary that the C++ framework will consume.

### 2.3 Existing C GUI Applications
- **Files:** `userspace/apps/sdk_explorer/main.c`, `userspace/apps/gui_demo/main.c`, `userspace/apps/desktop_shell/main.c`
- **Verification:** Both compile cleanly with `clang -target x86_64-pc-none-elf -ffreestanding -nostdlib` and link with `ld.lld -T userspace/linker.ld`.
- **Verdict:** C applications must continue to compile and run without modification.

### 2.4 Existing C++ Runtime & Toolchain
- **Files:** `build.ps1:L4044-L4047`, `atoms/userspace/runtime/libatoms_cpp.a`, `atoms/userspace/runtime/crt0.o`
- **Toolchain:** Clang++ `-std=c++20 -target x86_64-unknown-none-elf -ffreestanding -fno-exceptions -fno-rtti -O2`.
- **Verification:** Tested compilation and linking of `userspace/apps/apal_dashboard/main.cpp` using `crt0.o`, `libatoms_cpp.a`, and `libatoms_c.a`. Build exited with code 0.
- **Verdict:** Toolchain is capable of building full C++20 UI framework code.

---

## 3. Risk Analysis

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| Window Manager Duplication | High (compositor fighting, broken z-order) | Strictly enforce that `bos::Window` is a client controller that calls syscalls 16-23; kernel retains total ownership of window chrome, dragging, and compositing. |
| Memory Leaks / Double Free | Medium (memory exhaustion in Ring 3) | Enforce RAII; delete copy constructor/assignment; implement strict move semantics. |
| Broken C Compatibility | Critical (system regression) | Keep `userspace/libbos_gui` untouched; C++ framework lives in `sdk/include/bos/` and `sdk/src/bos/ui/`. |
| Exception / RTTI Overhead | High (linker errors on freestanding target) | Build strictly with `-fno-exceptions -fno-rtti`. Use `Result<T>` and return codes for error propagation. |

---

## 4. Files Involved in Upcoming Phase 1 Implementation

- **New Headers:**
  - `sdk/include/bos/types.hpp`
  - `sdk/include/bos/geometry.hpp`
  - `sdk/include/bos/events.hpp`
  - `sdk/include/bos/surface.hpp`
  - `sdk/include/bos/widget.hpp`
  - `sdk/include/bos/layout.hpp`
  - `sdk/include/bos/resource.hpp`
  - `sdk/include/bos/window.hpp`
  - `sdk/include/bos/application.hpp`
  - `sdk/include/bos/ui.hpp`
- **New Implementation Sources:**
  - `sdk/src/bos/ui/surface.cpp`
  - `sdk/src/bos/ui/widget.cpp`
  - `sdk/src/bos/ui/window.cpp`
  - `sdk/src/bos/ui/application.cpp`
- **New Demo Application:**
  - `userspace/apps/cpp_ui_demo/main.cpp`
- **Modified Build System:**
  - `build.ps1` (adding C++ UI build targets without modifying existing C targets)
