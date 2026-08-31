# PHASE 9 PATCH PLAN: SKIA 2D GRAPHICS ENGINE + BWE INTEGRATION

**Document ID:** ATRIX-PHASE9-PLAN-001  
**Phase:** TASK 3 — ARCHITECT TEAM  
**Target Subsystem:** Skia 2D Graphics Engine, CPU Rasterizer, BWE Surface Bridge & ATRIX Rendering Pipeline  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Architectural Scope & Target Pipeline

Phase 9 integrates the open-source Skia 2D graphics engine into ATOMS OS and establishes the software rasterization bridge into the BWE window manager and ATRIX browser:

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        ATRIX BROWSER / USERSPACE APP                   │
│                     (e.g., about:skia-test Omnibox)                    │
├────────────────────────────────────────────────────────────────────────┤
│                       Skia 2D Graphics Engine                          │
│     • SkCanvas (drawRect, drawRRect, drawCircle, drawLine, drawPath)   │
│     • SkPaint (Colors, Stroke/Fill, Alpha, Porter-Duff Blending)       │
│     • SkPath & SkMatrix (Bezier Geometry, Affine 3x3 Transforms)       │
│     • SkSurface (Raster Surface Memory Manager)                        │
├────────────────────────────────────────────────────────────────────────┤
│                       ATOMS Skia Graphics Adapter                      │
│       (AtomsSkiaSurface: Zero-Copy Direct Mapping to BWE Framebuffer)  │
├────────────────────────────────────────────────────────────────────────┤
│                           BWE Window Canvas                            │
│                 (32-bit ARGB/BGRA Window Pixel Buffer)                 │
├────────────────────────────────────────────────────────────────────────┤
│                 BWE Window Manager & Compositor Blitter                │
│                 (Damage Invalidation -> Screen Display)                │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Files to Create and Modify

### 2.1 Skia Core Headers (`third_party/skia/include/core/`)
- **Create:** `SkTypes.h`: Core types, fixed-point math macros, and alignment.
- **Create:** `SkColor.h`: `SkColor`, `SkColor4f`, `SkPMColor`, alpha extraction and packing.
- **Create:** `SkRect.h`: `SkRect`, `SkIRect` floating-point and integer rectangles.
- **Create:** `SkRRect.h`: `SkRRect` rounded rectangles with per-corner radii.
- **Create:** `SkMatrix.h`: 3x3 affine transformation matrix operations.
- **Create:** `SkPaint.h`: `SkPaint` stroke/fill styles, color, and blend modes.
- **Create:** `SkPath.h`: `SkPath` vector path geometry and verb sequence.
- **Create:** `SkImageInfo.h`: Color type, alpha type, dimensions, and row stride.
- **Create:** `SkCanvas.h`: High-level 2D drawing API.
- **Create:** `SkSurface.h`: Raster surface allocation and factory methods.
- **Create:** `SkBlendMode.h`, `SkClipOp.h`: Blend modes and clipping operators.

### 2.2 Skia CPU Rasterizer Implementation (`third_party/skia/src/core/`)
- **Create:** `SkColor.cpp`, `SkRect.cpp`, `SkRRect.cpp`, `SkMatrix.cpp`, `SkPaint.cpp`, `SkPath.cpp`, `SkImageInfo.cpp`, `SkCanvas.cpp`, `SkSurface.cpp`, `SkRasterizer.cpp`.

### 2.3 ATOMS Skia Adapter (`third_party/skia/include/adapter/` & `src/adapter/`)
- **Create:** `atoms_skia_adapter.h` & `atoms_skia_adapter.cpp`: Bridges BWE window canvas buffers directly to `SkSurface::MakeRasterDirect`.

### 2.4 Phase 9 Skia Verification Suite (`third_party/skia/tests/`)
- **Create:** `skia_test_suite.h` & `skia_test_suite.cpp`: Implements all 20 required deterministic test cases (clear, rect, circle, path, alpha blend, clip, transform, benchmark, etc.).

### 2.5 ATRIX Browser Integration & Build Configuration
- **Modify:** [`kernel/apps/atrix/atrix_browser.c`](file:///D:/Signatures_OS/kernel/apps/atrix/atrix_browser.c): Add `about:skia-test` and `about:skia` Omnibox routing.
- **Modify:** [`BUILD.gn`](file:///D:/Signatures_OS/BUILD.gn): Add `skia_core` static library and `skia_test_runner` executable.
- **Modify:** [`build.ps1`](file:///D:/Signatures_OS/build.ps1): Integrate Skia compilation into OS build pipeline.

---

## 3. Risk Assessment & Rollback Plan

- **Risk:** Pixel format mismatch resulting in red/blue channel swapping.
  - **Mitigation:** Comprehensive pixel format test with known test swatches (pure red `0xFFFF0000`, pure green `0xFF00FF00`, pure blue `0xFF0000FF`) verifying exact byte positions.
- **Risk:** Memory corruption or buffer overflow during path rasterization.
  - **Mitigation:** Strict clipping bounds enforcement in `SkRasterizer` ensuring no write occurs outside `[0, width)` and `[0, height)`.
- **Rollback:** Skia code is isolated in `third_party/skia/` and does not alter existing legacy graphics APIs.
