# BOS Graphics Engine V2 (BOGE V2) — Pure Rendering Engine

> **Module:** Core Rendering & Compositing Engine  
> **Status:** Phase 0 Architecture Frozen  
> **Target Path:** `kernel/graphics/BOGE/`  

---

## 1. Engine Responsibility

**BOGE V2** is strictly responsible for managing graphics memory primitives, caching window surfaces, and executing high-speed 2D compositing:
1. **Retained Surface Cache (`Surface/`):** Allocates and manages per-window system RAM backing bitmaps (`BOGE_Surface`).
2. **Command Ring Buffers (`API/`, `Core/`):** Dequeues asynchronous drawing commands from userspace applications without blocking compositing.
3. **Resource & Bitmap Caching (`Cache/`, `Image/`):** Pre-decodes BMP/PNG images and manages LRU eviction pools.
4. **Font Atlas (`Font/`):** Pre-renders ASCII/Unicode glyphs into a unified 256×256 texture atlas for fast UV blitting.
5. **Render Graph & Damage Math (`RenderGraph/`):** Evaluates Z-order occlusion culling and decomposes dirty rectangles into exact Y-X banded visible spans.
6. **Compositing Blitter (`Blitter/`):** Blits visible spans onto the global staging backbuffer using 64-bit unrolled memory copies and SIMD vector alpha blending.

---

## 2. Subdirectory Architecture

- `Core/` — Initialization (`BOGE_Initialize`), lifecycle, and global rendering context.
- `Renderer/` — Retained compositing loops (`BOGE_ComposeFrame`).
- `Surface/` — `BOGE_Surface` creation, destruction, and slab memory pooling.
- `Cache/` — LRU texture caching and memory slab allocators.
- `Font/` — ASCII/Unicode font rasterization and texture atlas builder.
- `Image/` — Uncompressed BMP/PNG decoders and pre-multiplied ARGB converters.
- `Blitter/` — High-speed 64-bit unrolled memory copying (`memset64`, `memcpy64`).
- `RenderGraph/` — Z-order sorting, occlusion culling, and Y-X span clipping math.
- `Effects/` — AME post-processing transformations (shadows, window fading).
- `API/` — Public kernel/userspace API wrappers and syscall dispatchers.
- `include/` — Internal and public BOGE V2 header definitions (`boge.h`).

---

## 3. Architectural Rule

> **"BOGE V2 shall never execute hardware I/O instructions (`inb`/`outb`), never touch physical VRAM MMIO mappings directly, and never invoke window client drawing callbacks during frame composition."**
