# NEXT AGENT HANDOFF — ATOMS OS OPENGL & ATOMS GRAPH 3D

## READING ORDER FOR A NEW AI

If you are a newly initialized AI agent working on this codebase with zero prior chat history, **follow this exact reading order BEFORE opening code or making changes**:

1. **`docs/opengl/NEXT_AGENT_HANDOFF.md`** (This Document) — Understand overall project status, architecture boundaries, and immediate priorities.
2. **`docs/opengl/OPENGL_MASTER_CONTEXT.md`** — Learn the full end-to-end pipeline (Application → Horse Engine → BWE → BGL → GL → Rasterizer → FBO → Present).
3. **`docs/opengl/PHASE_11_ATOMS_GRAPH_3D.md`** — Study the implementation, benchmark stages, and telemetry of the ATOMS GRAPH 3D application.
4. **`docs/opengl/OPENGL_FILE_MAP.md`** — Use as an index to open ONLY the source files relevant to your task without scanning the full tree.
5. **`docs/opengl/OPENGL_VERIFICATION_STATUS.md`** — Check existing empirical test suites (Phases 0–11) to avoid breaking working invariants.

---

## 1. Current Project State

- **System Context**: Signatures ATOMS OS — a 32-bit x86 bare-metal operating system kernel with standard graphics HAL (BSPE), window engine (BWE V2.0), desktop shell, and native software-rasterized OpenGL library (BGL).
- **OpenGL Engine Version**: OpenGL Phase 11 — **ATOMS GRAPH 3D** (Native 3D Benchmark & Stress Application).
- **Compilation Status**: `build.ps1` builds cleanly (0 compiler errors, exit code 0). Image builder updates `build/OS.img` (FAT32 partition).
- **Empirical Verification Status**: Automated QEMU test runs execute continuously and report `[PHASE11] ALL TESTS PASSED!`. All Phase 0–10 regression suites also pass cleanly.

---

## 2. What Was Most Recently Completed

- **OpenGL Phase 10 — Offscreen Rendering & Render-to-Texture (FBO Foundation)**:
  - FBO, Renderbuffer (RBO), Color/Depth/Stencil target attachments, feedback-loop guards, two-pass RTT, multi-target switching.
  - Verified by `kernel/debug/test_gl_phase10.c` (Tests A through V PASS).

- **OpenGL Phase 11 — ATOMS GRAPH 3D Benchmark Application**:
  - Full native 3D benchmark app registered under Horse Engine (`APP_ID_GRAPH_3D = 13`).
  - 10 procedural rendering stress stages (`kernel/apps/atoms_graph_3d/atoms_graph_scene.c`).
  - Real-time telemetry & metric collection (`atoms_graph_metrics.c`).
  - Deterministic `ATOMS GRAPH SCORE` formula calculation (`atoms_graph_benchmark.c`).
  - Side panel metrics UI overlay (`atoms_graph_ui.c`).
  - Verified by `kernel/debug/test_gl_phase11.c` (Tests A through R PASS).

---

## 3. What Is Currently Being Worked On

- Transitioning from core engine implementation to user-level application polish and UI workflow optimization for ATOMS GRAPH 3D.
- Preparing for post-Phase 11 features (e.g. enhanced lighting, texture filtering options, advanced app features, or userspace GL wrappers).

---

## 4. Exact Documents to Read Next

- Core Architecture & End-to-End Pipeline: [OPENGL_MASTER_CONTEXT.md](file:///d:/Signatures_OS/docs/opengl/OPENGL_MASTER_CONTEXT.md)
- Complete Source File Index: [OPENGL_FILE_MAP.md](file:///d:/Signatures_OS/docs/opengl/OPENGL_FILE_MAP.md)
- ATOMS GRAPH 3D Benchmark Application Deep Dive: [PHASE_11_ATOMS_GRAPH_3D.md](file:///d:/Signatures_OS/docs/opengl/PHASE_11_ATOMS_GRAPH_3D.md)
- Verification & Test Suite Matrix: [OPENGL_VERIFICATION_STATUS.md](file:///d:/Signatures_OS/docs/opengl/OPENGL_VERIFICATION_STATUS.md)

---

## 5. Exact Source Files to Inspect Depending on Task

| Task Subsystem | Primary Source Files to Inspect |
| :--- | :--- |
| **Public GL API additions** | `kernel/graphics/gl/gl.h`, `kernel/graphics/gl/gl.c` |
| **GL State / Context Mutators** | `kernel/graphics/gl/gl_state.h`, `kernel/graphics/gl/gl_state.c` |
| **Rasterization / Triangle Rendering** | `kernel/graphics/gl/gl_rasterizer.c`, `kernel/graphics/gl/gl_triangle.c`, `kernel/graphics/gl/gl_fragment.c` |
| **Texture Allocations & Filtering** | `kernel/graphics/gl/gl_texture.c`, `kernel/graphics/gl/gl_sampler.c` |
| **FBO / Offscreen Targets** | `kernel/graphics/gl/gl_fbo.c`, `kernel/graphics/gl/gl_fbo.h` |
| **BGL Context / Drawable Binding** | `kernel/graphics/bgl/bgl_context.c`, `kernel/graphics/bgl/bgl_drawable.c` |
| **BWE Window Engine & Pointer Safety** | `kernel/wm/bwe/src/bwe_window.c`, `kernel/wm/bwe/include/bwe.h` |
| **ATOMS GRAPH 3D App Logic** | `kernel/apps/atoms_graph_3d/atoms_graph_3d.c`, `atoms_graph_benchmark.c` |
| **ATOMS GRAPH 3D Procedural Scenes** | `kernel/apps/atoms_graph_3d/atoms_graph_scene.c` |
| **ATOMS GRAPH 3D Telemetry & UI** | `kernel/apps/atoms_graph_3d/atoms_graph_metrics.c`, `atoms_graph_ui.c` |
| **Automated Testing Suite** | `kernel/debug/test_gl_phase11.c`, `kernel/debug/test_gl_phase10.c` |

---

## 6. Important Architecture That MUST NOT Be Broken

1. **Working Phase 0–10 GL Infrastructure**: DO NOT rewrite working OpenGL rasterization, clipping, or FBO pipeline code unless reproducing a specific proven bug.
2. **Public OpenGL API Contract**: Application code (like ATOMS GRAPH 3D or DOOM) MUST render via standard `gl*` and `bgl*` public calls. DO NOT bypass public APIs by writing directly into framebuffer memory.
3. **BWE `user_data` Pointer Guard**: Windows in BWE use `win->user_data` to store either context pointers OR non-pointer integer `app_id` values (e.g. `(void*)(uintptr_t)13`). Any window cleanup logic MUST check `(uintptr_t)win->user_data > 4096` before calling `kfree()` to prevent fatal heap corruption crashes.
4. **BGL Client Insets vs Window Bounds**: BGL context drawables map to window *client* bounds, which inset the outer window geometry to account for titlebars and borders.

---

## 7. Known Unresolved Questions / Issues

- **Benchmark Duration vs QEMU TCG Speed**: Software-rasterized 3D rendering under QEMU TCG emulation takes ~0.5s per frame for dense stages. Automated test suites (`test_gl_phase11.c`) use accelerated step times (`stage_duration_ms = 1ms` or `10ms`) to verify stage progression and scoring logic in seconds, whereas real desktop interactive runs use 30s per stage (5 minutes total).
- **UI Responsiveness & Refresh**: The 2D side panel telemetry overlay renders directly on the BGL drawable framebuffer. When window resizing occurs, drawable buffers reallocate dynamically.

---

## 8. Recommended Next Action

1. Verify build output by running `.\build.ps1` and updating the image via `.\build\image_builder.exe build\boot.bin build\stage2.bin build\kernel.bin build\OS.img`.
2. Run automated test suite verification using QEMU serial output.
3. Consult `docs/opengl/PHASE_11_ATOMS_GRAPH_3D.md` before adding new 3D app features or tweaking telemetry parameters.
