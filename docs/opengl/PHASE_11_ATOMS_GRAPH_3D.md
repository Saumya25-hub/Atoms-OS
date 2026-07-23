# PHASE 11 — ATOMS GRAPH 3D BENCHMARK & RENDERING STRESS APPLICATION

## 1. Overview & Objectives

OpenGL Phase 11 delivers **ATOMS GRAPH 3D**, the first native 3D benchmark and graphics rendering stress application for ATOMS OS. It launches inside a real ATOMS OS BWE desktop window, executes a progressive 10-stage 3D rendering workload, gathers real-time telemetry metrics, computes a deterministic `ATOMS GRAPH SCORE`, and renders a side-panel telemetry interface.

---

## 2. Subsystem Architecture & Modules

All source files are located in `kernel/apps/atoms_graph_3d/`:

### `atoms_graph_3d.c` / `atoms_graph_3d.h`
- **Application Controller & Window Lifecycle**:
  - `atoms_graph_3d_launch(uint32_t* out_win_id)`: Creates a 900x512 desktop window (`BOS_CreateSurface`), assigns `win->user_data = (void*)APP_ID_GRAPH_3D`, attaches window render and event callbacks, and starts the benchmark controller.
  - `atoms_graph_3d_close(void)`: Clears `win->user_data = NULL`, cleans up benchmark resources, and destroys the window surface (`BOS_DestroySurface`).
  - `atoms_graph_3d_pump_frame(float delta_ms)`: Advances the benchmark state machine frame by frame.

### `atoms_graph_benchmark.c` / `atoms_graph_benchmark.h`
- **Benchmark Controller & Stage State Machine**:
  - Manages stage transitions (Stages 1 through 10), animation angle accumulation, timing, and score computation.
  - `atoms_graph_benchmark_init(...)`: Sets stage duration (default 30,000 ms = 30s per stage; fast-stepping in automated test suites).
  - `atoms_graph_benchmark_step(...)`: Renders current 3D stage frame, updates telemetry metrics, renders side panel UI, and handles automatic stage progression.

### `atoms_graph_renderer.c` / `atoms_graph_renderer.h`
- **BGL Renderer & Window Surface Binding**:
  - Insets the 900x512 window client area into a 260px right-side UI panel and a 640x512 3D rendering viewport.
  - Instantiates `BGLDrawable` and `BGLContext`.
  - Executes `bglSwapBuffers` to present rendered frames.

### `atoms_graph_scene.c` / `atoms_graph_scene.h`
- **Procedural 3D Scene Generators (10 Stages)**:
  - Generates 3D meshes, procedural textures, and FBO offscreen targets without external asset dependencies.
  - `atoms_graph_scene_init_gl(...)`: Generates procedural 64x64 checkerboard texture and 256x256 FBO offscreen color/depth targets.
  - `atoms_graph_scene_render_stage(...)`: Executes GL drawing logic based on the active stage index.

### `atoms_graph_metrics.c` / `atoms_graph_metrics.h`
- **Telemetry Metric Gathering & Scoring Engine**:
  - Tracks FPS (min, max, current, average), frame times (current, average, worst spike), triangle counts (current, total, peak), draw calls, heap memory usage and growth delta (`mem_delta_kb`).
  - Calculates deterministic `ATOMS GRAPH SCORE`:
    $$\text{Score} = \max\left(0, (\text{Avg FPS} \times 100) + (\text{Total Triangles} / 1000) + (\text{Passed Stages} \times 500) - (\text{Worst FT ms} \times 10) - (\text{Memory Growth KB} \times 2)\right)$$

### `atoms_graph_ui.c` / `atoms_graph_ui.h`
- **2D Panel UI & Result Card Renderer**:
  - Renders 260px right side panel on the BGL color buffer.
  - Displays real-time FPS, Frame Time, Triangles/sec, Draw Calls, Resolution (640x512), Heap Memory used/delta, Stage progress bar, and final benchmark result overlay.

---

## 3. The 10 Progressive Benchmark Stages

| Stage # | Stage Name | Description & Workload Characteristics |
| :---: | :--- | :--- |
| **1** | **Baseline Geometry** | Single rotating 3D RGB cube (12 triangles, basic MVP transformation). |
| **2** | **Geometry Scaling** | Grid of 36 rotating 3D cubes (432 triangles, multiple draw calls). |
| **3** | **Depth Complexity** | Concentric overlapping cubes with depth testing enabled (`GL_DEPTH_TEST`). |
| **4** | **Texture Workload** | 3D cubes sampled with procedural 64x64 checkerboard texture (`GL_TEXTURE_2D`). |
| **5** | **Multi-Object Scene** | Orbiting ring of cubes surrounding a central rotating core. |
| **6** | **Render-to-Texture** | Offscreen FBO rendering pass applied as texture to a 3D spinning cube. |
| **7** | **RTT Stress** | Multi-pass offscreen FBO rendering with recursive texture sampling. |
| **8** | **High Geometry Stress** | Dense 3D terrain grid mesh (high vertex & triangle density). |
| **9** | **Combined Pipeline Stress** | FBO + Texture + Depth + Lighting + Alpha Blending combined pipeline torture. |
| **10** | **Stability Run** | High-speed sustained rendering pass testing memory and state stability. |

---

## 4. Current Real State Categorization

### IMPLEMENTED
- Full ATOMS GRAPH 3D application source code (`kernel/apps/atoms_graph_3d/*`).
- Desktop integration with icon, start menu entry (`APP_ID_GRAPH_3D = 13`), and window shell creation.
- 10-stage procedural 3D scene generators.
- Real-time telemetry metric counters and score formula.
- Complete Phase 11 automated verification suite (`kernel/debug/test_gl_phase11.c`).

### VERIFIED
- **QEMU Serial Telemetry**: Automated test suite (`run_phase11_gl_verification_suite()`) runs in QEMU and outputs `[PHASE11] ALL TESTS PASSED!` for Tests A through R.
- **Window Lifecycle & Cleanup**: Clean window destruction without memory leaks or pointer corruption crashes.
- **Scoring Engine**: Formula evaluates deterministically and yields non-zero benchmark scores.
- **Phase 0–10 Regression**: All regression tests from Phase 0 through Phase 10 pass cleanly alongside Phase 11.

### OBSERVED
- Software rasterization in QEMU TCG mode renders smoothly; high vertex stages execute within expected TCG frame time limits.
- Automated test suites step stages using fast timeouts (`1ms`–`10ms`), whereas desktop interactive launch uses 30s per stage.

### REMAINING / NEEDS REVIEW (For Future Sprints)
- **Side Panel Overlay Formatting**: Text clipping or layout margin adjustments when window is resized below 900x512.
- **Userspace GL Extensions**: Exposing ATOMS GRAPH 3D as a userspace executable once ELF userspace GL libraries are finalized.
