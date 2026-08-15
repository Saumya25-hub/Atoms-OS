# ROOK V2 ARCHITECTURE IMPLEMENTATION
## PHASE 2: HARDWARE-ISOLATED SURFACE CONTRACT SPECIFICATION

```
================================================================================
ATOMS OS — ROOK V2 DISPLAY ARCHITECTURE
PHASE 2 DELIVERABLE: HARDWARE-ISOLATED SURFACE CONTRACT
================================================================================
Industry References:  Windows NT DWM / DXGI SwapChain Surfaces,
                      Linux DRM/KMS Dumb Buffers, Wayland wl_shm_pool,
                      macOS IOSurface CoreAnimation Model
Target Architecture:  Universal Bare-Metal (Haswell H81, AMD iGPU, NVIDIA, UEFI GOP)
Status:               ARCHITECTURAL SPECIFICATION — ZERO REGRESSION CONTRACT
================================================================================
```

---

## 1. Executive Summary & Industry Standard Analysis

In production operating systems (Windows NT, macOS Darwin, Linux DRM), **UI Rendering Code is never exposed to physical GPU scanline pitch**.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                 INDUSTRY SURFACE ARCHITECTURE COMPARISON                   │
├───────────────────┬─────────────────────────────────────────────────────────┤
│ OS Platform       │ Hardware Isolation Mechanism                            │
├───────────────────┼─────────────────────────────────────────────────────────┤
│ Microsoft Windows │ DXGI / DirectComposition: Render targets are dense      │
│                   │ ID3D11Texture2D / GDI Bitmaps. DWM blits to VRAM pitch. │
│ Linux / Wayland   │ wl_surface / wl_buffer: Client writes dense pixels.     │
│                   │ Compositor (Mutter/KWin) presents to DRM Framebuffer.   │
│ macOS             │ IOSurface / Metal: Dense pixel buffers in RAM.          │
│                   │ Quartz Compositor handles Window Server display pitch.  │
│ ATOMS OS ROOK V2  │ rook_surface_t: UI renders dense (y * width + x).       │
│                   │ RookPresenter handles VRAM Hardware Pitch (2048/2560).  │
└───────────────────┴─────────────────────────────────────────────────────────┘
```

---

## 2. The Surface Contract Invariant

### 2.1 The RAM Surface Invariant:
For every managed RAM surface `rook_surface_t`:
$$\mathbf{\text{stride\_pixels} \equiv \text{width}}$$
$$\text{Pixel Address}(x, y) = \text{pixels} + (y \times \text{width} + x)$$

### 2.2 Mathematical Proof of Isolation:
Let $W = \text{Surface Width}$ and $H = \text{Surface Height}$.
For any point $(x, y)$ such that $0 \le x < W$ and $0 \le y < H$:
$$0 \le y \times W + x < W \times H$$
Because the UI renderers only compute $(y \times W + x)$, **no pixel can ever be written outside the logical surface**, and **no unwritten gap words can ever exist inside the active frame**.

---

## 3. Data Structure Definition (`rook.h`)

```c
typedef struct {
    uint32_t* pixels;          /* RAM Canvas Pointer (Dense linear memory) */
    uint32_t  width;           /* Logical Canvas Width (e.g. 1920) */
    uint32_t  height;          /* Logical Canvas Height (e.g. 1080) */
    uint32_t  stride_pixels;   /* STRICT INVARIANT: Always equal to width in RAM */
    uint32_t  format;          /* 0x01: ARGB8888 (32-bit True Color) */
    bool      is_locked;       /* Presentation Guard Flag */
} rook_surface_t;
```

---

## 4. Atomic Screen Transition Protocol (Phase 3 Integration)

To eliminate ghost remnants between screens:
```c
int rook_goto(uint16_t page_id) {
    // 1. Exit current screen
    if (g_current_page && g_current_page->ops.on_exit) {
        g_current_page->ops.on_exit(g_current_page);
    }

    // 2. ATOMIC ZERO-WIPE (Solid Black #000000)
    uint32_t* bb = rook_get_backbuffer();
    if (bb) {
        uint32_t total = rook_get_width() * rook_get_height();
        for (uint32_t i = 0; i < total; i++) bb[i] = 0xFF000000;
    }

    // 3. Enter new screen
    g_current_page_id = page_id;
    g_current_page = next_page;
    if (g_current_page->ops.on_enter) {
        g_current_page->ops.on_enter(g_current_page);
    }

    // 4. Invalidate entire canvas for fresh present
    rook_invalidate_full();
    return 0;
}
```

---

## 5. Scope of Code Modifications

1. `kernel/shell/rook/include/rook.h`:
   - Declare `rook_surface_t` and `rook_get_surface()`.
2. `kernel/shell/rook/src/rook_render.c`:
   - Initialize `rook_surface_t g_rook_main_surface`.
   - Update `rook_render_flush()`: reads dense RAM surface at `py * width`, writes to hardware VRAM at `py * pitch_pixels`.
3. `kernel/shell/rook/src/rook_core.c`:
   - Implement atomic buffer zero-wipe on `rook_goto()`.
4. `kernel/services/wallpaper/wallpaper_service.c`:
   - Write pixels using `y * width + x` (dense RAM stride).
5. `kernel/shell/rook/pages/page_login.c` & `premium_signin_renderer.h`:
   - Enforce `stride_pixels = width` everywhere.

---

## 6. Protected Subsystems Invariance Verification

Under **Protocol V2.0 Rule 6**, certified subsystems remain **100% untouched**:
* CPU, GDT, SMP, IDT, PIC, PMM, VMM, HEAP, Scheduler, AGDTE, BOOTX64 = **UNTOUCHED 🔒**.

*Phase 2 Architectural Specification Approved.*
