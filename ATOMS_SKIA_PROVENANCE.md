# ATOMS OS SKIA PROVENANCE & SOURCE ATTRIBUTION

**Document ID:** ATRIX-PHASE9-PROVENANCE-001  
**Phase:** Phase 9 — Skia Source Provenance Record  
**Date:** 2026-08-26  

---

## 1. Classification & Attribution Matrix

| Component / File | Category | Upstream Project / Author | License | Description of ATOMS Adaptation |
|:---|:---|:---|:---|:---|
| `SkTypes.h` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Core types and math utility definitions. |
| `SkColor.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | 32-bit ARGB/BGRA color math & packing. |
| `SkRect.h` / `SkRRect.h` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Axis-aligned and rounded rectangle geometry. |
| `SkMatrix.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | 3x3 affine transformation matrix math. |
| `SkPaint.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Stroke/fill styling, blend modes, and color state. |
| `SkPath.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Vector path verbs (Move, Line, Quad, Cubic, Close). |
| `SkImageInfo.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Color type, alpha type, and dimension descriptors. |
| `SkCanvas.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | High-level 2D drawing command dispatcher. |
| `SkSurface.h` / `.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | Direct and heap raster surface management. |
| `SkRasterizer.cpp` | **ADAPTED CODE** | Google Skia | BSD 3-Clause | CPU scanline converter and subpixel blitter. |
| `atoms_skia_adapter.h` / `.cpp` | **ATOMS INTEGRATION** | ATOMS OS Team | Proprietary / ATOMS | Direct memory bridge connecting SkSurface to BWE canvas. |
| `skia_test_suite.h` / `.cpp` | **ATOMS ORIGINAL** | ATOMS OS Team | Proprietary / ATOMS | 20-test deterministic verification suite. |
