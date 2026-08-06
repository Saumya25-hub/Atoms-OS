# BOS Cursor Engine (BCE) V1.0 — Technical Architecture & Pipeline Specification

## 1. Overview & Architectural Goals

The **BOS Cursor Engine (BCE) V1.0** is the production-grade cursor subsystem for **ATOMS OS**, architected to match the precision, capability, and responsiveness of the Microsoft Windows `win32k` cursor manager and the Linux `DRM/KMS` + `libXcursor` pipeline.

Prior implementations relied on static synthetic ASCII bitmaps or hardcoded software overlays. BCE V1.0 provides:
- **Native Windows `.cur` & `.ani` Binary Decoders** (DIB ARGB + 1-bit AND transparency masks; RIFF `ACON` multi-frame animation containers).
- **Direct GPU Hardware Overlay Upload** via VMware SVGA II FIFO command pipelines (`SVGA_CMD_DEFINE_CURSOR`).
- **1000Hz Hardware Interrupt Driven Animation Driver** running inside PIT Timer IRQ 0 (`timer.c`).
- **Dual-Sync Hardware/Software Presenter Pipeline** (`BSPE_CursorPresenter_UpdateBitmap`).
- **Context-Aware App Launching Spinner States** (3.5-second timed loading window).

---

## 2. Directory & Source Layout

```
kernel/graphics/cursor/
├── include/
│   ├── bos_cursor.h             # BCE Master Subsystem API & State Machine
│   ├── bos_cur_loader.h         # Windows .CUR Binary DIB Decoder Header
│   ├── bos_ani_loader.h         # Windows RIFF .ANI Container Decoder Header
│   ├── bos_cursor_theme.h       # Theme Registry & Cursor Type Bindings
│   ├── bos_cursor_animation.h   # Animation Timeline & Sequence State
│   ├── bos_cursor_hal.h         # GPU Hardware Layer Bridge
│   ├── bos_cursor_cache.h       # LRU Cursor Bitmap Cache
│   ├── bos_cursor_diag.h        # Diagnostic & Telemetry Autopsy Logger
│   └── w11_cursor_assets.h      # Embedded Windows 11 Concept Theme C Bytes
├── core/
│   ├── bos_cursor.c             # Subsystem Init, Active Type, Timer Tick Engine
│   ├── cur_loader.c             # Bottom-Up DIB BGRA to ARGB & AND Mask Parser
│   ├── ani_loader.c             # RIFF ACON/'fram' Icon Container Parser
│   ├── cursor_theme.c           # Theme Allocator & Asset Resolver
│   ├── cursor_animation.c       # Animation Frame Sequence Scheduler
│   ├── cursor_hal.c             # VMware SVGA II / GPU Hardware Bridge
│   ├── cursor_cache.c           # LRU Cache Manager
│   └── cursor_diag.c            # Autopsy & Performance Logging
└── tests/
    └── cursor_tests.c           # 16-Scenario Automated Boot Certification Suite
```

---

## 3. End-to-End Pipeline & Workflow Diagram

```
+-----------------------------------------------------------------------------+
|                          INPUT & APP EVENTS                                |
|  - Desktop Icon Double Click (desktop_shell.c)                             |
|  - Application Launch Request (horse_engine.c, explorer.c)                 |
+-----------------------------------------------------------------------------+
                                     |
                                     v
+-----------------------------------------------------------------------------+
|                     BCE STATE MACHINE (bos_cursor.c)                        |
|  - bos_cursor_set_active_type(BCE_CURSOR_APPSTARTING)                      |
|  - Set g_anim_start_ms = timer_get_ticks()                                  |
|  - Set g_anim_timeout_ms = 3500 (3.5s loading window)                      |
+-----------------------------------------------------------------------------+
                                     |
                                     v
+-----------------------------------------------------------------------------+
|                 1000Hz PIT TIMER INTERRUPT IRQ 0 (timer.c)                 |
|  - Fires every 1 ms continuously on hardware IRQ 0                          |
|  - Calls bos_cursor_tick()                                                 |
|  - Computes: frame_idx = (elapsed_ms / 40) % frame_count (25 FPS spin)     |
+-----------------------------------------------------------------------------+
                                     |
                +--------------------+--------------------+
                |                                         |
                v                                         v
+-------------------------------+         +-----------------------------------+
| GPU HARDWARE OVERLAY (HAL)    |         | SOFTWARE PRESENTATION PIPELINE    |
| - bos_cursor_hal_upload_frame |         | - BSPE_CursorPresenter_UpdateBitmap|
| - Writes to VMware SVGA FIFO  |         | - Updates s_bitmap in RAM         |
|   (SVGA_CMD_DEFINE_CURSOR 19) |         | - Redraws on 60Hz compositor pass |
| - Flushes via SVGA_REG_SYNC   |         |   (bspe_cursor_present.c)         |
+-------------------------------+         +-----------------------------------+
```

---

## 4. Key Subsystem Modules & File Details

### A. Windows `.cur` Binary Decoder (`cur_loader.c`)
- Parses 6-byte CUR header (`idReserved`, `idType=2`, `idCount`).
- Locates DIB `BITMAPINFOHEADER` (32-bit ARGB / 24-bit RGB / 8-bit indexed).
- Converts bottom-up BGRA scanlines to native top-down 32-bit ARGB (`0xAARRGGBB`).
- Processes 1-bit AND transparency masks to generate alpha channels for zero-alpha legacy bitmaps.

### B. Windows RIFF `.ani` Container Decoder (`ani_loader.c`)
- Validates `RIFF` magic and `ACON` form type.
- Reads `anih` chunk (`BCE_ANIHEADER` struct containing frame count, step count, jiffy rates).
- Parses `LIST` chunks of type `'fram'` and extracts individual embedded `'icon'` chunks.
- Passes each icon chunk to `bos_cur_parse()` to extract 37 high-resolution frames into `bce_cursor_t->frames[]`.

### C. VMware SVGA II Hardware Upload (`gpu_drv_vmware.c`)
- Function: `vmware_set_cursor_image()`
- Reserves words in the VMware Command FIFO via `fifo_reserve()`.
- Writes `SVGA_CMD_DEFINE_CURSOR` (Command ID 19) payload:
  - `id = 1`, `hotspotX`, `hotspotY`, `width`, `height`, `depth = 1`, `bpp = 32`.
  - AND mask bitplane bytes.
  - 32-bit ARGB color pixel array.
- Commits FIFO words via `fifo_commit()` and triggers GPU sync (`SVGA_REG_SYNC`).

### D. 1000Hz Hardware Interrupt Animation Driver (`timer.c` & `bos_cursor.c`)
- In `timer.c` (`timer_tick_handler` on IRQ 0), `bos_cursor_tick()` is called on every 1ms timer interrupt.
- `bos_cursor_tick()` uses `timer_get_ticks()` for hardware-calibrated millisecond timing.
- Calculates elapsed time: `elapsed_ms = (now_ms - g_anim_start_ms)`.
- Advances `g_current_frame_idx` every 40ms (25 FPS smooth rotation).
- When a 3.5s loading window expires, automatically reverts active cursor back to `BCE_CURSOR_ARROW`.

### E. Software Compositor Dual-Sync Bridge (`bspe_cursor_present.c`)
- Function: `BSPE_CursorPresenter_UpdateBitmap()`
- Whenever `bos_cursor_hal_upload_frame()` updates a new animated frame, it calls `BSPE_CursorPresenter_UpdateBitmap()` to copy the new frame into `s_bitmap`.
- This ensures that 60Hz compositor redraw passes (`BSPE_CursorPresenter_OnCompositorRedraw`) draw the exact current animated frame rather than overwriting it with stale static bitmaps.

---

## 5. Enum & Data Structure Reference

```c
typedef enum {
    BCE_CURSOR_ARROW = 0,        /* Windows 11 Concept Arrow (arrow.cur) */
    BCE_CURSOR_HAND,             /* Link Selection Pointer (hand.cur) */
    BCE_CURSOR_IBEAM,            /* Text Beam Selector (ibeam.cur) */
    BCE_CURSOR_CROSSHAIR,        /* Precision Crosshair (crosshair.cur) */
    BCE_CURSOR_APPSTARTING,      /* App Launching Spinner (appstarting.ani) */
    BCE_CURSOR_WAIT,             /* Busy Spinning Ring (wait.ani) */
    BCE_CURSOR_HELP,             /* Help Select (help.cur) */
    BCE_CURSOR_UNAVAILABLE,      /* Disabled / No Entry (no.cur) */
    BCE_CURSOR_MOVE,             /* Move / Size All (sizeall.cur) */
    BCE_CURSOR_RESIZE_NS,        /* Vertical Resize (sizens.cur) */
    BCE_CURSOR_RESIZE_WE,        /* Horizontal Resize (sizewe.cur) */
    BCE_CURSOR_RESIZE_NWSE,      /* Diagonal Resize 1 (sizenwse.cur) */
    BCE_CURSOR_RESIZE_NESW,      /* Diagonal Resize 2 (sizenesw.cur) */
    BCE_CURSOR_TYPE_COUNT
} bce_cursor_type_t;
```

---

## 6. How Future Developers / AI Agents Should Modify or Extend BCE

1. **Adding a New Cursor Theme**:
   - Add `.cur` or `.ani` binary files to `MOUSE-ICO/<theme_name>/`.
   - Update `scratch/gen_w11_assets.py` to generate byte arrays in `w11_cursor_assets.h`.
   - Add theme name resolution in `cursor_theme.c` (`bos_cursor_theme_load()`).

2. **Triggering Cursor Animations for New UI Elements**:
   - To trigger an animated spinner during heavy I/O or app loading:
     ```c
     extern uint32_t bos_cursor_set_active_type(uint32_t type);
     bos_cursor_set_active_type(4 /* BCE_CURSOR_APPSTARTING */);
     ```
   - The 3.5s timed animation will automatically run on IRQ 0 and revert back to `arrow.cur`.

3. **Porting to New GPU Drivers (Intel / VirtIO / AMD / NVIDIA)**:
   - Implement `.set_cursor_image` in the driver's `bos_gpu_driver_t` ops structure.
   - The HAL in `cursor_hal.c` will automatically forward frame uploads to the primary GPU driver.

---

## 7. Version Control Tag & Commit

- **GitHub Release Tag**: `mouse-engine-v2-ani-cur`
- **Commit Hash**: `3f9e065`
- **Repository**: `Saumya25-hub/SignaturesOS`
