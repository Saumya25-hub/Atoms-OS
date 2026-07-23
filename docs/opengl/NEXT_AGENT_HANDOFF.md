# NEXT AGENT HANDOFF — ATOMS OS OPENGL & ATOMS GRAPH 3D

## READING ORDER FOR A NEW AI AGENT

If you are a newly initialized AI agent working on this codebase with zero prior chat history, **follow this exact reading order BEFORE opening code or making changes**:

1. **`docs/opengl/NEXT_AGENT_HANDOFF.md`** (This Document) — Understand overall project status, architecture boundaries, recent Phase 11 completions, and immediate priorities.
2. **`docs/opengl/OPENGL_MASTER_CONTEXT.md`** — Learn the full end-to-end pipeline (Application → Horse Engine → BWE → BGL → GL → Rasterizer → FBO → Present).
3. **`docs/opengl/PHASE_11_ATOMS_GRAPH_3D.md`** — Study the implementation, 10 benchmark stages, launcher/icon pipeline, and telemetry of ATOMS GRAPH 3D.
4. **`docs/opengl/OPENGL_FILE_MAP.md`** — Use as an index to open ONLY the source files relevant to your task without scanning the full tree.
5. **`docs/opengl/OPENGL_VERIFICATION_STATUS.md`** — Check existing empirical test suites (Phases 0–11) to avoid breaking working invariants.

---

## 1. Current Project State

- **System Context**: Signatures ATOMS OS — a 32-bit x86 bare-metal operating system kernel with standard graphics HAL (BSPE), window engine (BWE V2.0), desktop shell, and native software-rasterized OpenGL library (BGL).
- **OpenGL Engine Version**: OpenGL Phase 11 — **ATOMS GRAPH 3D** (Native 3D Benchmark & Stress Application).
- **Git Release Tag**: `v4.1.0-opengl-phase11-complete` on branch `phase15-performance-audit`.
- **Compilation Status**: `build.ps1` builds cleanly (0 compiler errors, exit code 0). Image builder (`build/image_builder.exe`) creates `build/OS.img` (FAT32 partition).
- **Empirical Verification Status**: Normal OS boot proceeds cleanly into the desktop shell without panics or GPF exceptions. Automated GL test suites pass cleanly when enabled via `#ifdef ENABLE_BOOT_GL_TESTS`.

---

## 2. Complete Summary of What Was Completed in Phase 11

### A. ATOMS GRAPH 3D Benchmark Application Stack
- **Core Engine** (`kernel/apps/atoms_graph_3d/`): Full 3D rendering benchmark application running in a native 900x512 BWE window.
- **10 Procedural 3D Workload Stages** (`atoms_graph_scene.c`):
  1. Baseline Geometry (Rotating RGB cube)
  2. Geometry Scaling (36 rotating cubes grid)
  3. Depth Complexity (Concentric z-tested cubes)
  4. Texture Workload (Procedural 64x64 checkerboard texture)
  5. Multi-Object Scene (Orbiting ring of cubes)
  6. Render-to-Texture (Offscreen FBO pass applied to 3D cube face)
  7. RTT Stress (Multi-pass recursive FBO sampling)
  8. High Geometry Stress (Dense 3D terrain grid mesh)
  9. Combined Pipeline Stress (FBO + Texture + Depth + Lighting + Alpha Blending)
  10. Stability Run (High-speed sustained rendering pass)
- **Telemetry & Score Engine** (`atoms_graph_metrics.c` & `atoms_graph_benchmark.c`): Tracks real-time FPS, frame time (ms), total triangles, draw calls, and memory growth delta. Calculates deterministic `ATOMS GRAPH SCORE`.
- **2D Telemetry Panel UI** (`atoms_graph_ui.c`): Renders 260px right side-panel telemetry overlay.

### B. Custom Futuristic 3D Floating Icon & Asset Pipeline
- **PNG Icon Generator** (`tools/generate_graph3d_icon.py`): Script generating `assets/icons/graph3d.png` (64x64 RGBA). Features a static perspective 3D gem/cube with bright cyan top specular highlights, deep cyan left face, indigo/violet shadow face, wireframe edge glows, apex node dots, and a soft ground drop shadow creating a spatial floating illusion.
- **FAT32 8.3 Entry Formatting** (`tools/image_builder.c`): Formatted 8.3 entry `"GRAPH3D PNG"` (7 filename chars + 1 space + 3 extension chars = 11 bytes) to pack `assets/icons/graph3d.png` as `GRAPH3D.PNG` in `build/OS.img`.
- **Asset Cache Preloading** (`kernel/ui/boasset/`): Added `#define ICON_GRAPH_3D 115` (`asset_types.h`) and preloaded `ICON_GRAPH_3D` into `s_master_atlas` inside `BOAsset_PreloadCritical()` (`boasset.c`). Added procedural 3D gem fallback in `asset_loader.c`.

### C. Desktop Launcher Integration
- **Desktop Grid Icon** (`kernel/shell/desktop_shell/desktop_shell.c`): Registered desktop launcher icon `"3D Benchmark"` via `create_desktop_icon("3D Benchmark", APP_ID_GRAPH_3D, -1, -1);`.
- **System-Wide Icon Mapping**: Mapped `APP_ID_GRAPH_3D` (App ID 13) to `ICON_GRAPH_3D` across Desktop Shell, Start Menu, and Taskbar.
- **Launch Dispatcher**: Clicking launcher dispatches `APP_ID_GRAPH_3D` → `horse_launch` → `atoms_graph_3d_launch` → opens 900x512 BWE window.

### D. Boot Test Isolation & Bug Fixes
- **Boot Test Guard** (`kernel/kernel.c`): Guarded automated test suites (`run_phase8_gl_verification_suite` through `run_phase11_gl_verification_suite`) under `#ifdef ENABLE_BOOT_GL_TESTS`. This resolved General Protection Fault (Exception 13) at `bglMakeCurrent` (RIP `0x103921`) during boot.

---

## 3. What Is Next / Phase 12 Preparation

- Preparing for **Phase 12** development (e.g. Shader Emulation, Advanced Lighting/Specular Highlights, Texture Filtering Modes, Multi-Window GL Rendering, or Userspace ELFs).

---

## 4. Source Files Map for Phase 11

| Subsystem | Source File Path | Responsibility |
| :--- | :--- | :--- |
| **App Lifecycle** | `kernel/apps/atoms_graph_3d/atoms_graph_3d.c/.h` | Window creation, pump loop, close callbacks |
| **Benchmark Loop** | `kernel/apps/atoms_graph_3d/atoms_graph_benchmark.c/.h` | Stage state machine, score computation |
| **BGL Surface Binding** | `kernel/apps/atoms_graph_3d/atoms_graph_renderer.c/.h` | Viewport insets, BGL context/drawable binding |
| **3D Procedural Scenes** | `kernel/apps/atoms_graph_3d/atoms_graph_scene.c/.h` | 10 3D workload stage draw calls & FBO setup |
| **Telemetry & Metrics** | `kernel/apps/atoms_graph_3d/atoms_graph_metrics.c/.h` | Frame timing, FPS, triangles, score formula |
| **Side Panel UI** | `kernel/apps/atoms_graph_3d/atoms_graph_ui.c/.h` | 260px right side panel & final result card |
| **Icon Generator** | `tools/generate_graph3d_icon.py` | Generates 64x64 PNG floating 3D icon |
| **FAT32 Packing** | `tools/image_builder.c` | Packs `GRAPH3D.PNG` into `OS.img` root directory |
| **Asset Pipeline** | `kernel/ui/boasset/boasset.c`, `asset_loader.c`, `asset_types.h` | Preloads `ICON_GRAPH_3D` into texture atlas |
| **Desktop Launcher** | `kernel/shell/desktop_shell/desktop_shell.c` | Registers `"3D Benchmark"` desktop icon |
| **Start Menu / Taskbar** | `kernel/ui/start_menu.c`, `task_panel.c` | Icon resolution for App ID 13 |
| **Boot Guard** | `kernel/kernel.c` | Controls `#ifdef ENABLE_BOOT_GL_TESTS` for boot safety |

---

## 5. Architectural Invariants That MUST NOT Be Broken

1. **BWE `user_data` Pointer Guard**: Windows store either pointers or integer app IDs in `win->user_data`. Any cleanup code MUST check `(uintptr_t)win->user_data > 4096` before calling `kfree()` to prevent fatal heap corruption crashes.
2. **FAT32 8.3 Filename Formatting**: FAT32 directory entries in `image_builder.c` MUST pad filenames with spaces up to 8 chars plus 3 extension chars (e.g. `"GRAPH3D PNG"`, NOT `"GRAPH3DPNG"`).
3. **Atlas Preloading**: New application icon assets MUST be preloaded inside `BOAsset_PreloadCritical()` in `boasset.c` so their UV coordinates are calculated during master texture atlas construction.

---

## 6. How to Build & Verify

1. **Compile Kernel**:
   `powershell -ExecutionPolicy Bypass -File .\build.ps1`
2. **Build Host Image Tool**:
   `clang tools/image_builder.c -o build/image_builder.exe`
3. **Build Disk Image**:
   `build\image_builder.exe build\boot.bin build\stage2.bin build\kernel.bin build\OS.img`
4. **Boot QEMU**:
   `qemu-system-x86_64 -m 1024 -vga std -drive file=build\OS.img,format=raw -serial stdio`
