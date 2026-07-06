# ATOMS OS BOGE V2 & BSPE Phase 1 Migration Strategy

> **Document Type:** Step 4 Migration Strategy & API Wrapping Schedule  
> **Status:** Phase 1 Engineering Kickoff  
> **Core Commandment:** **DO NOT Rewrite the OS. DO NOT Break Existing Systems. 100% Backward Compatibility.**  

---

## 1. Executive Strategy & Phasing Principles

To implement **Phase 1 (BSPE Presentation Foundation)** without breaking the working desktop, login screen, boot animation, or existing userspace applications, ATOMS OS utilizes the **Strangler Fig Migration Pattern**. 

Instead of deleting or rewriting BOGE V1 files (`graphics.c`, `bwe_compositor.c`, `vbe.c`), we build the clean **BSPE** subsystem alongside them under `kernel/graphics/BSPE/`. We then convert existing V1 public rendering functions into **transparent backward-compatibility wrappers** that internally route execution to the new BSPE presentation pipeline.

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ STRANGLER FIG MIGRATION: BACKWARD COMPATIBILITY WRAPPER FLOW                                       │
├────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ [Existing App / UI Code] ──► Calls BOVISUAL_Graphics_SwapFull(&back_vram)                          │
│                                      │                                                             │
│                         [Transparent Wrapper in graphics.c]                                        │
│                                      │                                                             │
│                         ├──► 1. Wraps ram_fb in a temporary BOGE_StagingFrame struct               │
│                         └──► 2. Calls BSPE_PresentFrame(&staging_frame)                            │
│                                      │                                                             │
│                         [New BSPE Presentation Engine: Phase 1]                                    │
│                                      │                                                             │
│                         ├──► Executes Dual-Page Damage Union: Damage(N) U Damage(N-1)              │
│                         ├──► Blits ONLY damaged pixels (~4 KB) across MMIO bus to VRAM Page 1      │
│                         └──► Executes atomic VSync page flip via Bochs VBE I/O ports!              │
└────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. File Categorization Matrix for Phase 1

### 2.1 What Files Change (Modified strictly to insert wrapper redirection or hook points)
| File Path | Nature of Modification | Why It Changes in Phase 1 |
| :--- | :--- | :--- |
| **`bovisual/Graphics/graphics.c`** | Modify `BOVISUAL_Graphics_SwapFull()` body. | Convert from raw full-frame memcpy into a transparent wrapper calling `BSPE_PresentFrame()`. |
| **`kernel/wm/bwe/renderer/bwe_compositor.c`** | Modify `BWE_ComposeFrame()` and `BVCursor_Draw()`. | Hook `BSPE_PresentFrame()` at frame end; decouple software cursor when BSPE hardware cursor is active. |
| **`kernel/drivers/video/vbe/vbe.c`** | Modify `vbe_init()` and `vbe_swap_page()`. | Register Bochs VBE functions into the `BSPE_DisplayDriver` HAL interface table upon kernel boot. |
| **`kernel/shell/rook/pages/page_boot.c`** | Modify boot animation loop. | Ensure boot animation frames pass through `BSPE_PresentFrame()` for tear-free 60 FPS transitions. |

### 2.2 What Files Stay Untouched in Phase 1 (100% Frozen Until Phase 2–4)
| File Path | Why It Remains Untouched in Phase 1 | Future Migration Phase |
| :--- | :--- | :---: |
| **`kernel/wm/surface/surface.c`** | Surface pool and immediate-mode UI callbacks continue working without alteration. | **Phase 2** *(Retained Surfaces)* |
| **`kernel/wm/bwe/src/bwe_window.c`** | Window registration, coordinate tracking, and titlebar chrome drawing remain unchanged. | **Phase 2** *(Window Manager)* |
| **`kernel/wm/bwe/src/bwe_core.c`** | BOHeart 60Hz master clock and input pumping continue driving the frame loop. | **Phase 2** *(Decoupled Clock)* |
| **`bovisual/Text/text.c`** | ASCII string bitmask scanning continues functioning until font atlas is built. | **Phase 3** *(Font Atlas)* |
| **`bovisual/Images/images.c`** | Uncompressed BMP pixel decoding continues functioning until bitmap cache is built. | **Phase 3** *(Bitmap Cache)* |
| **`bovisual/Renderer/renderer.c`** | UI widget rendering primitives continue drawing to `ram_fb`. | **Phase 2** *(Retained Widgets)* |
| **`bovisual/Drawing/drawing.c`** | Bresenham line and circle primitives continue calling `PutPixel`. | **Phase 2** *(Blitter)* |
| **`kernel/shell/rook/pages/page_login.c`** | Login UI presentation logic continues executing without modification. | **Phase 2** *(Retained Login)* |
| **`kernel/shell/desktop_shell/desktop_shell.c`**| Desktop wallpaper copying continues executing without modification. | **Phase 2** *(Wallpaper Cache)* |
| **`kernel/ame/src/ame_core.c`** | AME animation ticking and curve evaluators continue functioning identically. | **Phase 2** *(AME Integration)* |

---

## 3. API Lifecycle & Evolution Schedule

### 3.1 APIs Becoming Transparent Wrappers (Phase 1)
These V1 functions retain their exact C signatures and compilation contracts. Their internal implementation is replaced with redirection to BSPE:
- **`BOVISUAL_Graphics_SwapFull(BOImage* buffer)`**: Internally constructs a `BOGE_StagingFrame` struct from `buffer->data` and invokes `BSPE_PresentFrame()`. **Callers in window manager, login, and boot animation require zero code changes!**
- **`vbe_swap_page(uint32_t y_offset)`**: Internally routed through the active `BSPE_DisplayDriver->swap_page()` HAL function pointer.

### 3.2 APIs Becoming Deprecated (Annotated with `@deprecated`, removed in Phase 4)
These functions continue working in Phase 1 but generate compiler warnings to discourage new usage:
- **`BOVISUAL_Graphics_SwapFull`**: Deprecated in favor of direct `BSPE_PresentFrame()` calls.
- **`BVCursor_Draw`**: Deprecated in favor of asynchronous hardware cursor register updates (`BSPE_Cursor_SetPosition`).
- **`BWE_AddCompositorDirtyRect`**: Deprecated in favor of `BOGE_Surface_UnlockBufferAndInvalidate()`.

### 3.3 APIs Replaced Later (Phase 2 & 3)
- **`BOVISUAL_Draw_String`**: Replaced in Phase 3 by `BOGE_BlitGlyphString()` (Font Atlas).
- **`BOVISUAL_Draw_BMP`**: Replaced in Phase 3 by `BOGE_BlitBitmap()` (Bitmap Cache).
- **`BOS_CreateSurface`**: Replaced in Phase 2 by `BOGE_Surface_Create()` (Retained Backing Bitmaps).
