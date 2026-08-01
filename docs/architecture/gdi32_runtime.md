# 🏛️ GDI32.sll V1.0 Architectural Specification

> **Subsystem:** GDI32.sll V1.0 Graphics API Runtime  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Kernel  
> **Layer:** Ring 3 Win32 / ATOMS Standard Graphics API Runtime  

---

## 1. Executive Summary & Architecture

**GDI32.sll V1.0** is the official Ring 3 Graphics API Library for Signatures OS / ATOMS OS. Informed by Windows GDI32.dll, Cairo Graphics, Skia, Direct2D, X11, and Wayland, GDI32.sll provides standard Win32 2D drawing abstractions (`HDC`, `HBITMAP`, `HPEN`, `HBRUSH`, `HFONT`) while delegating all hardware rasterization and surface blitting directly to **AGP V1.0** (ATOMS Graphics Platform).

```text
 ┌─────────────────────────────────────────────────────────────┐
 │                Ring 3 Application Code                      │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Win32 GDI API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       GDI32.sll                             │
 │  ├── 1. Device Context Engine ├── 11. BitBlt Engine         │
 │  ├── 2. Pen Engine            ├── 12. StretchBlt Engine     │
 │  ├── 3. Brush Engine          ├── 13. AlphaBlend Engine     │
 │  ├── 4. Font Engine           ├── 14. Geometry Engine       │
 │  ├── 5. Text Rendering Engine ├── 15. Color Management      │
 │  ├── 6. Bitmap Engine         ├── 16. Surface Manager       │
 │  ├── 7. Image Engine          ├── 17. Double Buffer Engine  │
 │  ├── 8. Region Engine         ├── 18. Printing Stub         │
 │  ├── 9. Clipping Engine       ├── 19. Stock Object Manager  │
 │  └── 10. Painting Engine      └── 20. Diagnostics Engine    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ AGP GPU Translation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │              AGP (ATOMS Graphics Platform V1.0)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ BAR Application Runtime Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                 BOS Application Runtime (BAR)               │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Window Management
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                        USER32.sll                           │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Kernel Syscalls & GPU Driver
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                   x86_64 Kernel & Hardware                  │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Directory Tree Layout (`userspace/libs/gdi32/`)

```text
userspace/libs/gdi32/
├── include/
│   ├── gdi32_types.h
│   ├── gdi32_api.h
│   └── gdi32_public.h
├── core/
│   └── gdi32_runtime.c
├── dc/
│   └── gdi32_dc.c
├── pens/
│   └── gdi32_pen.c
├── brushes/
│   └── gdi32_brush.c
├── fonts/
│   └── gdi32_font.c
├── text/
│   └── gdi32_text.c
├── bitmap/
│   └── gdi32_bitmap.c
├── images/
│   └── gdi32_image.c
├── regions/
│   └── gdi32_region.c
├── clipping/
│   └── gdi32_clipping.c
├── painting/
│   └── gdi32_painting.c
├── blit/
│   └── gdi32_blit.c
├── alpha/
│   └── gdi32_alpha.c
├── paths/
│   └── gdi32_paths.c
├── geometry/
│   └── gdi32_geometry.c
├── colors/
│   └── gdi32_colors.c
├── surfaces/
│   └── gdi32_surfaces.c
├── printing/
│   └── gdi32_printing.c
├── diagnostics/
│   └── gdi32_diagnostics.c
├── tests/
│   └── gdi32_certification_tests.c
└── docs/
    └── gdi32_runtime.md
```

---

## 3. Core Engine Responsibilities

1. **Device Context Engine**: `HDC` allocation, save/restore state stack (`SaveDC`, `RestoreDC`).
2. **Pen Engine**: Line style, thickness, and color pens (`CreatePen`).
3. **Brush Engine**: Solid, hatch, and pattern brushes (`CreateSolidBrush`, `CreatePatternBrush`).
4. **Font Engine**: Font metrics, typeface selection, font handle allocation (`CreateFont`).
5. **Text Rendering Engine**: Glyph run translation to `AGP_DrawGlyphRun` (`TextOut`, `ExtTextOut`).
6. **Bitmap Engine**: Offscreen bitmap surface allocation (`CreateBitmap`, `CreateCompatibleBitmap`).
7. **Image Engine**: Image decoding and format conversion bridge.
8. **Region Engine**: Complex polygonal and rectangular region manipulation (`CreateRectRgn`, `CombineRgn`).
9. **Clipping Engine**: Viewport and region clip state (`SelectClipRgn`, `IntersectClipRect`).
10. **Painting Engine**: Vector shape rendering (`Rectangle`, `Ellipse`, `Polygon`, `FillRect`).
11. **BitBlt Engine**: Fast hardware memory surface blitting (`BitBlt`).
12. **StretchBlt Engine**: Surface scaling and stretch blitting (`StretchBlt`).
13. **AlphaBlend Engine**: Per-pixel alpha blending and transparency (`AlphaBlend`, `TransparentBlt`).
14. **Geometry Engine**: Point, line, and path geometric math.
15. **Color Management**: RGB and HSL color space conversion and `COLORREF` macros.
16. **Surface Manager**: Buffer surface creation and pitch management.
17. **Double Buffer Engine**: Flicker-free buffered paint context (`BeginBufferedPaint`, `EndBufferedPaint`).
18. **Printing Stub Runtime**: Print queue and spooler abstraction stubs.
19. **Stock Object Manager**: Built-in system pens, brushes, and stock fonts (`BLACK_PEN`, `WHITE_BRUSH`).
20. **Diagnostics Engine**: Object handle table leak audit, draw call profiler, VRAM memory meter.
