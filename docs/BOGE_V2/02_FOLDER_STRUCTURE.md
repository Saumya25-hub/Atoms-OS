# 02. Folder Structure & Engine Modularization

> **Module:** Production Directory Architecture  
> **Status:** Phase 0 Frozen  
> **Target Path:** `d:\Signatures_OS\kernel\graphics\`  

---

## 1. Purpose

To prevent spaghetti code, circular dependencies, and ambiguous ownership, the ATOMS OS graphics stack is organized into two distinct physical directory trees under `kernel/graphics/`: **`BOGE/`** and **`BSPE/`**. Every folder has a strict, single architectural responsibility.

---

## 2. Complete Production Folder Tree

```
kernel/
└── graphics/
    ├── BOGE/                         <── BOGE V2 (Rendering Engine)
    │   ├── Core/                     <── Engine initialization, lifecycle, state management
    │   ├── Renderer/                 <── Retained 2D compositor & rasterization loops
    │   ├── Surface/                  <── BOGE_Surface management & backing bitmap pools
    │   ├── Cache/                    <── Resource caching (bitmaps, glyphs, wallpapers)
    │   ├── Font/                     <── Font parsing, metrics, and texture atlas builder
    │   ├── Image/                    <── BMP/PNG decoders & pre-multiplied ARGB converters
    │   ├── Blitter/                  <── High-speed 64-bit unrolled & SIMD memory blitting
    │   ├── RenderGraph/              <── Z-order sorting, occlusion culling & damage clipping
    │   ├── Effects/                  <── AME integration (shadows, blur, color matrices)
    │   ├── API/                      <── Public kernel/userspace API wrappers & syscalls
    │   └── include/                  <── Internal and public BOGE V2 header definitions
    │
    └── BSPE/                         <── BSPE (Presentation Engine)
        ├── Present/                  <── Present queue, frame pacing & presentation loops
        ├── Swapchain/                <── Double / Triple buffer swapchain state managers
        ├── Damage/                   <── Dual-page VRAM damage history trackers
        ├── FramePacer/               <── VSync synchronization & timer pacing engines
        ├── Cursor/                   <── Hardware cursor plane & software fallback overlay
        ├── DisplayHAL/               <── Hardware Abstraction Layer for video drivers
        ├── Drivers/                  <── Driver backends (Bochs VBE, VGA, VESA, GPU HAL)
        ├── Debug/                    <── Diagnostic HUD overlays & telemetry collectors
        └── include/                  <── Internal and public BSPE header definitions
```

---

## 3. Subdirectory Responsibilities & Ownership Matrix

### 3.1 BOGE V2 Subdirectories

| Folder | Responsibility & Architectural Rule | Memory Ownership |
| :--- | :--- | :--- |
| **`Core/`** | Manages engine startup (`BOGE_Initialize`), shutdown, and global rendering state. | Global BOGE context struct. |
| **`Renderer/`** | Implements the retained compositing loop (`BOGE_ComposeFrame`). Never calls client draw functions directly. | Read-only access to surfaces; writes to Staging Buffer. |
| **`Surface/`** | Allocates, resizes, and destroys `BOGE_Surface` structs and their private backing bitmaps. | Owns all system RAM window backing bitmaps. |
| **`Cache/`** | Manages LRU eviction and memory pooling for decoded bitmaps and glyph atlases. | Owns texture cache memory slabs. |
| **`Font/`** | Rasterizes ASCII/Unicode fonts into a unified texture atlas upon boot or font load. | Owns `BOGE_FontAtlas` texture memory. |
| **`Image/`** | Decodes raw image files into uncompressed 32-bit ARGB texture buffers. | Temporary decode buffers. |
| **`Blitter/`** | Low-level memory copying (`memset64`, `memcpy64`, SIMD vector blending). Contains zero UI logic. | Zero state; operates on passed pointers. |
| **`RenderGraph/`** | Computes visible spans, clips dirty rectangles, and eliminates occluded windows before blitting. | Owns transient frame clip lists. |
| **`Effects/`** | Applies post-processing transformations driven by AME (shadow rendering, window fading). | Transient effect scratch buffers. |
| **`API/`** | Validates arguments and dispatches userspace requests (`BOS_SetText`, `BOS_Update`) to command queues. | Zero state; syscall interface layer. |

### 3.2 BSPE Subdirectories

| Folder | Responsibility & Architectural Rule | Memory Ownership |
| :--- | :--- | :--- |
| **`Present/`** | Receives staging frames from BOGE V2 and queues them for display presentation. | Owns `BSPE_PresentQueue` ring buffer. |
| **`Swapchain/`** | Manages front, back, and staging buffer acquisition, rotation, and release. | Owns swapchain metadata & buffer pointers. |
| **`Damage/`** | Computes dual-page VRAM damage history: $\text{Damage}(N) \cup \text{Damage}(N-1)$. | Owns `g_page_damage[2]` history arrays. |
| **`FramePacer/`** | Aligns presentation with monitor VSync intervals or precision hardware timers. | Owns timer IRQ handles & frame timestamps. |
| **`Cursor/`** | Controls hardware cursor registers (X, Y, Image) or manages asynchronous software sprite fallback. | Owns 32×32 cursor sprite memory. |
| **`DisplayHAL/`** | Abstract interface decoupling BSPE from physical video hardware. | Owns HAL function pointer tables. |
| **`Drivers/`** | Hardware-specific implementations (e.g. Bochs VBE I/O port writers, VESA BIOS extensions). | Owns MMIO / I/O port mapping addresses. |
| **`Debug/`** | Renders real-time FPS, damage count, and memory telemetry HUD directly onto presentation buffers. | Owns debug HUD overlay buffers. |

---

## 4. Architectural Rules for Code Placement

1. **No Cross-Inclusion:** Files in `BOGE/Renderer/` shall never include headers from `BSPE/Drivers/`. All communication must flow through `BSPE/include/bspe.h`.
2. **No Hardware Calls in BOGE:** BOGE V2 code shall never execute `inb`/`outb` or MMIO writes. All hardware interaction is strictly isolated within `BSPE/Drivers/`.
3. **No UI Logic in BSPE:** BSPE code shall never inspect window titles, button states, or UI layout coordinates. It operates exclusively on raw memory buffers and dirty rectangles.
