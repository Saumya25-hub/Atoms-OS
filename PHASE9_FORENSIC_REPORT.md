# PHASE 9 FORENSIC REPORT: SKIA 2D GRAPHICS ENGINE + BWE INTEGRATION

**Document ID:** ATRIX-PHASE9-FORENSIC-001  
**Phase:** TASK 1 & TASK 2 — FORENSIC INVESTIGATION  
**Target Subsystem:** Skia 2D Graphics Engine, CPU Rasterizer, BWE Surface Bridge & ATRIX Rendering Pipeline  
**Standard:** Rule 0 Phase Isolation (Investigate ➔ Plan ➔ Patch ➔ Certify)  
**Date:** 2026-08-26  

---

## 1. Executive Forensic Summary

A thorough forensic audit was performed across the ATOMS OS display hardware, BWE window manager (`kernel/wm/bwe/`), surface allocator (`kernel/gui/surface/`), framebuffer layer (`kernel/drivers/display/`), and userspace C/C++ runtime foundations (`userspace/runtime/`).

The audit revealed that:
1. **BWE Surface Architecture & Pixel Format:**
   - **Pixel Format:** 32-bit ARGB/XRGB (`0xAARRGGBB` in 32-bit unsigned integers; on little-endian x86_64, byte ordering in memory is Blue, Green, Red, Alpha = `kBGRA_8888_SkColorType` / `kN32_SkColorType`).
   - **Bits Per Pixel (BPP):** 32 bpp (4 bytes per pixel).
   - **Row Stride:** Stride is contiguous row-major (`width * 4` bytes per row).
   - **Coordinate System:** Top-left origin `(0, 0)` with positive X extending rightward and positive Y extending downward.
   - **Clipping Model:** Rectangular axis-aligned clipping regions with scissor testing.
   - **Buffer Ownership:** Surfaces allocate dedicated linear framebuffers accessible via `control_data.canvas.pixel_buffer` or mapped to userspace via `SYS_GUI_MAP_SURFACE`.
2. **Skia Engine Architecture (Task 2 Audit):**
   - The official open-source Skia 2D graphics engine provides a dedicated CPU software rasterizer that executes entirely on host memory buffers without GPU/driver dependencies.
   - Skia's software rasterization pipeline centers on:
     - `SkSurface`: Manages pixel memory via `SkSurface::MakeRasterDirect(SkImageInfo, void* pixels, size_t rowBytes)`.
     - `SkCanvas`: Provides drawing commands (`clear`, `drawRect`, `drawRRect`, `drawOval`, `drawCircle`, `drawLine`, `drawPath`, `clipRect`, `concat`, `save`, `restore`).
     - `SkPaint`: Encapsulates stroke/fill properties, anti-aliasing flags, blend modes, and color definitions.
     - `SkPath`: Vector geometry builder (verbs: MoveTo, LineTo, QuadTo, CubicTo, Close).
     - `SkMatrix`: 3x3 affine transformation matrix (Translate, Scale, Rotate).
     - `SkBlitter` / `SkRasterPipeline`: Scans vector spans, computes subpixel coverage, and performs alpha blending with destination pixels.
3. **Integration Point:**
   - Skia can draw directly into the BWE window's canvas framebuffer via a zero-copy memory adapter (`AtomsSkiaSurface`), followed by a call to `BWE_InvalidateWindow()` / `SYS_GUI_INVALIDATE` to present the rendered pixels.

---

## 2. BWE Graphics Subsystem Forensic Audit Matrix

| Parameter / Facility | BWE Specification | Skia Mapping | Status / Notes |
|:---|:---|:---|:---|
| **Pixel Format** | 32-bit `0xAARRGGBB` (BGRA byte order) | `kBGRA_8888_SkColorType` / `kPremul_SkAlphaType` | **100% Native Match** (Zero-copy direct rasterization) |
| **Pixel Stride** | `width * 4` bytes per scanline | `rowBytes = width * sizeof(uint32_t)` | **100% Aligned** |
| **Coordinate Space** | Top-Left `(0, 0)` Origin | Skia standard `(0, 0)` Top-Left Origin | **100% Aligned** |
| **Alpha Blending** | Premultiplied 8-bit Alpha | `SkMulDiv255Round` Porter-Duff compositing | **Compatible** |
| **Buffer Access** | Linear contiguous RAM buffer | `SkSurface::MakeRasterDirect` | **Zero-Copy Compatible** |
| **Dirty Tracker** | Invalidation flags (`is_dirty`) | `AtomsSkiaSurface::Flush()` triggers invalidate | **Compatible** |
| **Threading Model** | Single-threaded canvas paint | Skia CPU rasterizer is thread-safe per canvas | **Safe** |

---

## 3. Skia Source Audit & Dependency Assessment (Task 2 & Task 4)

| Skia Module / Component | Category | Role in Phase 9 | Current Status |
|:---|:---|:---|:---|
| **`SkCanvas` / `SkSurface`** | Core Graphics | Canvas API & surface memory management | **REQUIRED (Included)** |
| **`SkPaint` / `SkColor`** | Core Styles | Stroke, fill, color, and blend modes | **REQUIRED (Included)** |
| **`SkRect` / `SkRRect`** | Geometry | Rectangles and rounded rectangles | **REQUIRED (Included)** |
| **`SkPath` / `SkMatrix`** | Vector Path | Bezier curves, lines, affine transformations | **REQUIRED (Included)** |
| **`SkBlitter` / `SkScanline`** | Rasterizer | CPU scanline converter and pixel blitter | **REQUIRED (Included)** |
| **`AtomsSkiaSurface`** | Adapter | Bridge connecting Skia rasterizer to BWE surface | **REQUIRED (Included)** |
| **GPU / Vulkan / GL Backends** | Hardware Accel | GPU-accelerated drawing | **EXCLUDED (Phase 9 is CPU-only)** |
| **Text / HarfBuzz / FreeType** | Font Subsystem | Glyph shaping and layout | **OPTIONAL (Follow-on milestone)** |
| **Image Codecs (PNG/JPEG/WEBP)**| Codecs | Image decoding | **OPTIONAL (Follow-on milestone)** |

---

## 4. Root Causes & Technical Challenges

1. **Freestanding C++ Runtime Boundary:** Skia source code relies on modern C++20 language features (`std::string`, `std::vector`, `std::unique_ptr`, `operator new`/`delete`), which are provided by the Phase 7/8 userspace foundation.
2. **Buffer Lifetime & Safety:** When mapping BWE surface buffers, the adapter must ensure that buffer boundaries (`width * height * 4`) are strictly respected during rasterization to prevent out-of-bounds writes.

---

## 5. Risk Assessment & Safety Boundaries

- **Zero GPU Dependency:** The build must strictly compile the software CPU rasterizer without pulling in OpenGL, Vulkan, or DirectX headers.
- **Rule 0 Compliance:** Proof of rendering must be verified through real rasterization, pixel buffer checksums, and BWE presentation in ATRIX.
