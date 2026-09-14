# FORENSIC REPORT — BOS C++ UI FRAMEWORK PHASE 2 AUDIT
## Modern Visual Controls & PNG / Image Engine

**Target:** ATOMS OS Native C++ UI Framework  
**Scope:** Phase 2 Visual Controls, PNG/Deflate Decoding Engine, Visual State Architecture, 9-Slice Renderer, Asset Pipeline  
**Date:** 2026-09-10  
**Phase:** TASK 1 — FORENSIC TEAM (AUDIT ONLY — NO SOURCE CODE MODIFIED)

---

## 1. Executive Summary

Phase 1 established the authoritative C++20 foundation (`Application`, `Window`, `Surface`, `Widget`, `Event`, `Layout`, `Resource`, `Types`, `Geometry`) interfacing seamlessly with the frozen kernel GUI syscalls (16–23). Both C (`sdk_explorer.elf`) and C++ (`cpp_ui_demo.elf`) applications build cleanly with zero linker errors.

However, the framework currently lacks:
1. **A production-ready Control Library:** Controls beyond base `Widget` do not exist in C++ (only procedural C controls exist in `libbos_gui`).
2. **A Real PNG / Image Engine:** `Image` in `sdk/include/bos/resource.hpp` is a thin placeholder around an uncompressed `Bitmap`. There is no userspace PNG parser, Deflate decompressor, or alpha blending pipeline.
3. **A Universal Visual State Model:** Controls lack a standardized representation of `Normal`, `Hover`, `Pressed`, `Focused`, `Disabled`, and `Selected` states.
4. **9-Slice Scalable Surface Rendering:** Controls cannot scale corner borders and repeat/stretch centers for modern rounded cards, buttons, or window frames.
5. **DPI & Scale Awareness:** The framework operates purely on raw pixels without logical coordinate scaling (e.g. 100%, 125%, 150%, 200%).
6. **Icon Abstraction & Text Alignment:** Icons and glyphs cannot be dynamically sized, tinted, or aligned with labels.

---

## 2. Forensic Evidence & Subsystem Analysis

### 2.1 Existing PNG Decoding Assets & Kernel Code
- **Files Found:**
  - `kernel/media/bopawn/decoder/png/png_decoder.c`
  - `kernel/media/bopawn/decoder/png/util/inflate.c`
  - `kernel/media/bopawn/decoder/png/util/filters.c`
  - `docs/bopawn/04_png_decoder.md`
- **Analysis:**
  - ATOMS OS already possesses a completely native, freestanding RFC 1951 DEFLATE decompressor (`inflate.c`) and scanline unfiltering algorithm (`filters.c`).
  - Supports chunk parsing (`IHDR`, `IDAT`, `PLTE`, `IEND`).
  - Supports Truecolor (type 2), RGBA (type 6), Grayscale (type 0), Grayscale+Alpha (type 4), and Palette/Indexed (type 3).
  - In kernel space, it targets `kmalloc`/`kfree` and `BOSSurface`.
  - **Verdict:** We can adapt this proven, zero-dependency ATOMS OS implementation directly into userspace C++ (`sdk/src/bos/ui/png.cpp`), outputting directly into `bos::Bitmap` (32-bpp ARGB) with full alpha channel fidelity.

### 2.2 Existing PNG Asset Inventory in Repository
- **Directories:**
  - `assets/icons/` (14+ icons including `settings.png`, `terminal.png`, `explorer.png`, `calculator.png`, `computer.png`, `music.png`, `doom.png`, etc.)
  - `assets/icons/system/` (15 icons: `battery.png`, `battery_charging.png`, `wifi.png`, `volume.png`, `power.png`, `search.png`, etc.)
  - `assets/icons/navigation/` (6 icons: `home.png`, `folder.png`, `back.png`, `forward.png`, `refresh.png`, `up.png`)
  - `assets/icons/branding/` (`atoms_start.png`)
  - `BOOT(OS-ICO)/` (18 icons: `A-DIRVE.png`, `ATOM-drive.png`, `USBdrive.png`, `chat.png`, `lock.png`, `user.png`, `trash-bin.png`, etc.)
  - `BOOT-WALLAPPERS/` (10 wallpapers: `1.png` to `10.png`)
- **Verdict:** The repository has an abundant inventory of high-quality PNG icons and graphics ready to be consumed by the new image engine and showcase application.

### 2.3 Userspace File I/O Syscall Analysis
- **Files:** `atoms/userspace/runtime/include/atoms_syscall.h`, `userspace/runtime/c/src/atoms_syscall.c`
- **Mechanism:**
  - Standard POSIX `open`, `read`, `close`, `lseek`, `stat` are linked in `atoms/userspace/runtime/libatoms_c.a` via Syscalls `SYS_OPEN` (14), `SYS_READ` (15), `SYS_CLOSE` (25), `SYS_SEEK` (26).
  - Memory allocation (`malloc`, `free`, `realloc`) is provided by musl libc in `libatoms_c.a`, and `operator new`/`operator delete` are in `libatoms_cpp.a`.
- **Verdict:** The userspace runtime fully supports loading PNG files from disk as well as decoding from embedded in-memory buffers (`from_file` and `from_memory`).

### 2.4 Existing Procedural C Controls (libbos_gui)
- **Files:** `userspace/libbos_gui/include/bos_gui.h`, `userspace/libbos_gui/src/`
- **Controls Audited:**
  - `BOSButton`, `BOSLabel`, `BOSProgressBar`, `BOSCheckBox`, `BOSTextBox`, `BOSRadioButton`, `BOSScrollViewer`.
- **Verdict:** These are procedural C wrappers around kernel window panels. They must remain untouched for backward compatibility. The new C++ controls will provide an object-oriented, stateful, image-capable hierarchy extending `bos::Widget`.

### 2.5 Software Rendering & Surface Capabilities
- **File:** `sdk/include/bos/surface.hpp`, `sdk/src/bos/ui/surface.cpp`
- **Gaps Identified:**
  - `Surface` currently only supports solid `fill_rect`, `draw_rect`, `draw_line`, and 8x16 bitmap text `draw_string`.
  - Missing: `draw_image` with alpha compositing (`src_over`).
  - Missing: `draw_image_9slice` for scalable rounded borders.
  - Missing: `draw_rounded_rect` and `fill_rounded_rect`.
  - Missing: `blend_pixel` (alpha blending formula: `out = (src * a + dst * (255 - a)) / 255`).

---

## 3. Required Deliverables for Phase 2

1. **PNG Engine (`sdk/include/bos/png.hpp`, `sdk/src/bos/ui/png.cpp`)**
   - Freestanding Deflate/zlib decompressor + PNG chunk parser.
   - Decodes RGBA/RGB/Grayscale/Palette PNGs into 32-bpp ARGB `Bitmap`.
   - `Image::from_file()` and `Image::from_memory()`.
2. **Surface Rendering Extensions (`sdk/include/bos/surface.hpp`, `sdk/src/bos/ui/surface.cpp`)**
   - `blend_pixel()` for Porter-Duff source-over blending.
   - `draw_image()` with clipping and scaling (nearest / bilinear).
   - `draw_image_9slice()` using `Insets` border slicing (corners untouched, edges stretched, center stretched).
   - `fill_rounded_rect()` and `draw_rounded_rect()`.
3. **Visual State System (`sdk/include/bos/state.hpp`)**
   - Universal enum: `Normal`, `Hover`, `Pressed`, `Focused`, `Disabled`, `Selected`.
   - Reusable `ControlStyle` / `StateStyle` resolver with fallback mechanics.
4. **Icon & Scaling System (`sdk/include/bos/icon.hpp`, `sdk/include/bos/scale.hpp`)**
   - `Icon` with sizing, scaling, and state variations.
   - `Scale` helper for DPI factors (100%, 125%, 150%, 175%, 200%).
5. **Theme Foundation (`sdk/include/bos/theme.hpp`)**
   - Unified color tokens (Slate900, Slate800, Blue500, etc.), font metrics, radii, margins.
6. **Modern Control Library (`sdk/include/bos/controls/`, `sdk/src/bos/ui/controls/`)**
   - Basic: `Button`, `Label`, `TextBox`, `CheckBox`, `RadioButton`, `Toggle`.
   - Selection: `ListView`, `ComboBox`, `ProgressBar`.
   - Containers & Navigation: `Card`, `Sidebar`, `ScrollView`, `TabControl`, `ImageWidget`.
7. **Phase 2 Showcase Application (`userspace/apps/settings_demo/main.cpp`)**
   - ATOMS Settings & Personalization Demo inspired by the reference design.
   - Interactive sidebar, theme cards, all controls active, DPI scaling showcase.

---

## 4. Risk Analysis & Mitigation

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| PNG decompressor memory pressure | High memory use or allocation failure | Pre-calculate uncompressed buffer sizes; use bounded buffers; graceful fallback if invalid. |
| Alpha blending performance overhead | Frame drop during software compositing | Optimize inner pixel loop with dirty clipping bounds; avoid per-pixel div/mod; bitshift arithmetic. |
| C ABI / Phase 1 API Breakage | Regression in existing apps | Zero changes to `Widget` base API contract; only extend `Surface` and `Image`; keep `libbos_gui` untouched. |
| Memory leaks in dynamic controls | Resource exhaustion | Clean RAII destruction; `Bitmap` owns buffer with explicit freeing; Widget tree deletes child nodes properly. |

---

## 5. Verification Boundary

- Build static library `build/libbos_ui_cpp.a` containing all Phase 1 + Phase 2 sources.
- Build showcase application `build/settings_demo.elf`.
- Verify regression build of Phase 1 demo `build/cpp_ui_demo.elf`.
- Verify regression build of C app `build/sdk_explorer.elf`.
- Confirm 0 errors, 0 undefined symbols, and correct binary generation.
